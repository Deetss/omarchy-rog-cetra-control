#define _GNU_SOURCE
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/poll.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

#define OVERALL_DEADLINE_MS 12000
#define SDP_DEADLINE_MS 4000
#define CONNECT_DEADLINE_MS 5000
#define GETTER_DEADLINE_MS 2000

#define BUFFER_MAX 2048
#define FRAMES_PER_GETTER_MAX 64
#define SDP_RECORDS_MAX 64

static char g_timeout_buf[512] =
    "{\"status\":\"unavailable\",\"address\":\"\",\"error\":\"timeout\","
    "\"left\":null,\"right\":null,\"case\":null,\"mode\":\"unknown\","
    "\"left_charging\":null,\"right_charging\":null,\"case_charging\":null}\n";
static volatile size_t g_timeout_buf_len = sizeof(
    "{\"status\":\"unavailable\",\"address\":\"\",\"error\":\"timeout\","
    "\"left\":null,\"right\":null,\"case\":null,\"mode\":\"unknown\","
    "\"left_charging\":null,\"right_charging\":null,\"case_charging\":null}\n") - 1;

static void sigalrm_handler(int sig) {
    (void)sig;
    if (g_timeout_buf_len > 0) {
        ssize_t ret = write(STDOUT_FILENO, g_timeout_buf, g_timeout_buf_len);
        (void)ret;
    }
    _exit(1);
}

static void update_timeout_buf(const char *addr) {
    int n = snprintf(g_timeout_buf, sizeof(g_timeout_buf),
                     "{\"status\":\"unavailable\",\"address\":\"%s\",\"error\":\"timeout\","
                     "\"left\":null,\"right\":null,\"case\":null,\"mode\":\"unknown\","
                     "\"left_charging\":null,\"right_charging\":null,\"case_charging\":null}\n",
                     addr ? addr : "");
    if (n > 0 && (size_t)n < sizeof(g_timeout_buf)) {
        g_timeout_buf_len = (size_t)n;
    }
}

static int64_t get_boottime_ms(void) {
    struct timespec ts;
#if defined(CLOCK_BOOTTIME)
    if (clock_gettime(CLOCK_BOOTTIME, &ts) == 0) {
        return (int64_t)ts.tv_sec * 1000 + (ts.tv_nsec / 1000000);
    }
#endif
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        return (int64_t)ts.tv_sec * 1000 + (ts.tv_nsec / 1000000);
    }
    return 0;
}

static void arm_itimer_ms(int64_t ms) {
    if (ms <= 0) {
        ms = 1;
    }
    struct itimerval it;
    memset(&it, 0, sizeof(it));
    it.it_value.tv_sec = (time_t)(ms / 1000);
    it.it_value.tv_usec = (suseconds_t)((ms % 1000) * 1000);
    if (it.it_value.tv_sec == 0 && it.it_value.tv_usec == 0) {
        it.it_value.tv_usec = 1000;
    }
    setitimer(ITIMER_REAL, &it, NULL);
}

static void stop_timer_and_block_alrm(void) {
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGALRM);
    sigprocmask(SIG_BLOCK, &mask, NULL);
    struct itimerval zero_it;
    memset(&zero_it, 0, sizeof(zero_it));
    setitimer(ITIMER_REAL, &zero_it, NULL);
}

static bool validate_and_normalize_address(const char *input, char *output, size_t out_len) {
    if (!input || out_len < 18) {
        return false;
    }
    if (strlen(input) != 17) {
        return false;
    }
    for (int i = 0; i < 17; i++) {
        if (i == 2 || i == 5 || i == 8 || i == 11 || i == 14) {
            if (input[i] != ':') {
                return false;
            }
            output[i] = ':';
        } else {
            if (!isxdigit((unsigned char)input[i])) {
                return false;
            }
            output[i] = (char)toupper((unsigned char)input[i]);
        }
    }
    output[17] = '\0';
    return true;
}

static void emit_failure(const char *address, const char *err_code) {
    char out[512];
    int n = snprintf(out, sizeof(out),
                     "{\"status\":\"unavailable\",\"address\":\"%s\",\"error\":\"%s\","
                     "\"left\":null,\"right\":null,\"case\":null,\"mode\":\"unknown\","
                     "\"left_charging\":null,\"right_charging\":null,\"case_charging\":null}\n",
                     address ? address : "",
                     err_code ? err_code : "protocol");
    if (n > 0 && n <= 512) {
        fputs(out, stdout);
        fflush(stdout);
    }
}

static int format_success_json(char *buf, size_t buf_sz,
                               const char *address,
                               const uint8_t anc_payload[1],
                               const uint8_t power_payload[4],
                               const uint8_t chg_payload[2]) {
    char left_str[16];
    char right_str[16];
    char case_str[16];

    /* Power payload layout is [modecode, left, right, case] -> indexes 1, 2, 3 */
    if (power_payload[1] <= 100) {
        snprintf(left_str, sizeof(left_str), "%u", (unsigned int)power_payload[1]);
    } else {
        snprintf(left_str, sizeof(left_str), "null");
    }

    if (power_payload[2] <= 100) {
        snprintf(right_str, sizeof(right_str), "%u", (unsigned int)power_payload[2]);
    } else {
        snprintf(right_str, sizeof(right_str), "null");
    }

    if (power_payload[3] <= 100) {
        snprintf(case_str, sizeof(case_str), "%u", (unsigned int)power_payload[3]);
    } else {
        snprintf(case_str, sizeof(case_str), "null");
    }

    const char *mode_str = "unknown";
    if (anc_payload[0] == 0) {
        mode_str = "off";
    } else if (anc_payload[0] == 1) {
        mode_str = "anc";
    } else if (anc_payload[0] == 2) {
        mode_str = "ambient";
    }

    /* Charging mask: hexadecimal 00, 01, 10, 11 only (bit 0x01 left, 0x10 right) */
    const char *left_chg = "null";
    const char *right_chg = "null";
    if (chg_payload[0] == 0x00) {
        left_chg = "false";
        right_chg = "false";
    } else if (chg_payload[0] == 0x01) {
        left_chg = "true";
        right_chg = "false";
    } else if (chg_payload[0] == 0x10) {
        left_chg = "false";
        right_chg = "true";
    } else if (chg_payload[0] == 0x11) {
        left_chg = "true";
        right_chg = "true";
    }

    /* Case charging: 0 false, 1 true, else null */
    const char *case_chg = "null";
    if (chg_payload[1] == 0) {
        case_chg = "false";
    } else if (chg_payload[1] == 1) {
        case_chg = "true";
    }

    return snprintf(buf, buf_sz,
                    "{\"status\":\"ok\",\"address\":\"%s\",\"left\":%s,\"right\":%s,\"case\":%s,"
                    "\"mode\":\"%s\",\"left_charging\":%s,\"right_charging\":%s,\"case_charging\":%s}\n",
                    address ? address : "", left_str, right_str, case_str, mode_str, left_chg, right_chg, case_chg);
}

static void emit_success(const char *address,
                         const uint8_t anc_payload[1],
                         const uint8_t power_payload[4],
                         const uint8_t chg_payload[2]) {
    char out[1024];
    int n = format_success_json(out, sizeof(out), address, anc_payload, power_payload, chg_payload);
    if (n > 0 && n <= 1024) {
        fputs(out, stdout);
        fflush(stdout);
    }
}

static int acquire_user_lock(void) {
    int fd = socket(AF_UNIX, SOCK_DGRAM | SOCK_CLOEXEC, 0);
    if (fd < 0) {
        return -1;
    }
    struct sockaddr_un sun;
    memset(&sun, 0, sizeof(sun));
    sun.sun_family = AF_UNIX;
    sun.sun_path[0] = '\0';
    int n = snprintf(&sun.sun_path[1], sizeof(sun.sun_path) - 1,
                     "io.github.pavellizunov.rog-cetra-control.bt.%u",
                     (unsigned int)getuid());
    if (n <= 0 || (size_t)n >= sizeof(sun.sun_path) - 1) {
        close(fd);
        return -1;
    }
    socklen_t len = (socklen_t)(offsetof(struct sockaddr_un, sun_path) + 1 + (size_t)n);
    if (bind(fd, (struct sockaddr *)&sun, len) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

static int sdp_query_channel(const bdaddr_t *dst, int *out_channel, int64_t overall_deadline_ms, int64_t sdp_deadline_ms) {
    if (get_boottime_ms() >= sdp_deadline_ms || get_boottime_ms() >= overall_deadline_ms) {
        return -2;
    }

    sdp_session_t *session = sdp_connect(BDADDR_ANY, dst, SDP_RETRY_IF_BUSY);
    if (get_boottime_ms() >= sdp_deadline_ms || get_boottime_ms() >= overall_deadline_ms) {
        if (session) {
            sdp_close(session);
        }
        return -2;
    }
    if (!session) {
        return -1;
    }

    int64_t remain_ms = sdp_deadline_ms - get_boottime_ms();
    if (remain_ms > overall_deadline_ms - get_boottime_ms()) {
        remain_ms = overall_deadline_ms - get_boottime_ms();
    }
    if (remain_ms <= 0) {
        sdp_close(session);
        return -2;
    }
    struct timeval tv;
    tv.tv_sec = (time_t)(remain_ms / 1000);
    tv.tv_usec = (suseconds_t)((remain_ms % 1000) * 1000);
    setsockopt(session->sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(session->sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    uuid_t id;
    sdp_uuid16_create(&id, SERIAL_PORT_SVCLASS_ID);
    uint32_t range = 0x0000ffff;
    sdp_list_t *search = sdp_list_append(NULL, &id);
    sdp_list_t *attrs = sdp_list_append(NULL, &range);
    sdp_list_t *results = NULL;

    int rc = sdp_service_search_attr_req(session, search, SDP_ATTR_REQ_RANGE, attrs, &results);
    sdp_list_free(search, NULL);
    sdp_list_free(attrs, NULL);

    if (get_boottime_ms() >= sdp_deadline_ms || get_boottime_ms() >= overall_deadline_ms) {
        if (results) {
            for (sdp_list_t *it = results; it; it = it->next) {
                if (it->data) {
                    sdp_record_free((sdp_record_t *)it->data);
                }
            }
            sdp_list_free(results, NULL);
        }
        sdp_close(session);
        return -2;
    }

    if (rc < 0) {
        sdp_close(session);
        return -1;
    }

    int name_match_count = 0;
    int found_channel = -1;
    int record_count = 0;

    for (sdp_list_t *it = results; it; it = it->next) {
        record_count++;
        sdp_record_t *r = (sdp_record_t *)it->data;
        if (r) {
            char name[256] = {0};
            if (sdp_get_service_name(r, name, sizeof(name)) == 0) {
                if (strcmp(name, "Asus_APP") == 0) {
                    name_match_count++;
                    sdp_list_t *protos = NULL;
                    if (sdp_get_access_protos(r, &protos) == 0) {
                        int ch = sdp_get_proto_port(protos, RFCOMM_UUID);
                        if (ch >= 1 && ch <= 30) {
                            found_channel = ch;
                        }
                        for (sdp_list_t *p = protos; p; p = p->next) {
                            sdp_list_free(p->data, NULL);
                        }
                        sdp_list_free(protos, NULL);
                    }
                }
            }
            sdp_record_free(r);
        }
    }

    sdp_list_free(results, NULL);
    sdp_close(session);

    if (get_boottime_ms() >= sdp_deadline_ms || get_boottime_ms() >= overall_deadline_ms) {
        return -2;
    }

    /* Record count limit (64) is enforced post-traversal on parsed records.
     * Note: This does not bound internal heap allocations performed by libbluetooth
     * during raw SDP PDU parsing; a full bounded wire parser is out of scope to avoid
     * re-implementing BlueZ SDP library internals. */
    if (record_count > SDP_RECORDS_MAX) {
        return -1;
    }

    if (name_match_count != 1 || found_channel < 1 || found_channel > 30) {
        return -1;
    }

    *out_channel = found_channel;
    return 0;
}

static int rfcomm_connect_nonblocking(const bdaddr_t *dst, int channel, int64_t overall_deadline_ms) {
    int64_t now = get_boottime_ms();
    int64_t connect_deadline = now + CONNECT_DEADLINE_MS;
    if (connect_deadline > overall_deadline_ms) {
        connect_deadline = overall_deadline_ms;
    }

    if (now >= connect_deadline || now >= overall_deadline_ms) {
        return -2;
    }

    int sock = socket(AF_BLUETOOTH, SOCK_STREAM | SOCK_CLOEXEC, BTPROTO_RFCOMM);
    if (sock < 0) {
        return -1;
    }

    int flags = fcntl(sock, F_GETFL, 0);
    if (flags < 0 || fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0) {
        close(sock);
        return -1;
    }

    struct sockaddr_rc addr;
    memset(&addr, 0, sizeof(addr));
    addr.rc_family = AF_BLUETOOTH;
    bacpy(&addr.rc_bdaddr, dst);
    addr.rc_channel = (uint8_t)channel;

    int rc = connect(sock, (struct sockaddr *)&addr, sizeof(addr));
    if (get_boottime_ms() >= connect_deadline || get_boottime_ms() >= overall_deadline_ms) {
        close(sock);
        return -2;
    }
    if (rc == 0) {
        return sock;
    }
    if (rc < 0 && errno != EINPROGRESS) {
        close(sock);
        return -1;
    }

    while (1) {
        int64_t cur = get_boottime_ms();
        if (cur >= connect_deadline || cur >= overall_deadline_ms) {
            close(sock);
            return -2;
        }
        int timeout_ms = (int)(connect_deadline - cur);
        if (timeout_ms > (int)(overall_deadline_ms - cur)) {
            timeout_ms = (int)(overall_deadline_ms - cur);
        }
        struct pollfd pfd;
        pfd.fd = sock;
        pfd.events = POLLOUT;
        pfd.revents = 0;

        int prc = poll(&pfd, 1, timeout_ms);
        cur = get_boottime_ms();
        if (cur >= connect_deadline || cur >= overall_deadline_ms) {
            close(sock);
            return -2;
        }
        if (prc == 0) {
            close(sock);
            return -2;
        }
        if (prc < 0) {
            if (errno == EINTR) {
                continue;
            }
            close(sock);
            return -1;
        }

        if (pfd.revents & (POLLOUT | POLLERR | POLLHUP)) {
            int err = 0;
            socklen_t errlen = sizeof(err);
            if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &err, &errlen) < 0 || err != 0) {
                close(sock);
                if (err == ETIMEDOUT) {
                    return -2;
                }
                return -1;
            }
            return sock;
        }
    }
}

/* Returns 0 on success, -2 on timeout, -3 on protocol error, -1 on socket/IO error */
static int run_getter(int sock,
                      uint16_t command,
                      size_t expected_payload_len,
                      uint8_t *out_payload,
                      uint8_t *buf,
                      size_t *buf_len,
                      size_t buf_cap,
                      int64_t overall_deadline_ms) {
    int64_t now = get_boottime_ms();
    int64_t deadline = now + GETTER_DEADLINE_MS;
    if (deadline > overall_deadline_ms) {
        deadline = overall_deadline_ms;
    }
    if (now >= deadline || now >= overall_deadline_ms) {
        return -2;
    }

    uint8_t packet[4];
    packet[0] = 0xff;
    packet[1] = 0x00;
    packet[2] = (uint8_t)((command & 0xff) << 1);
    packet[3] = (uint8_t)((command >> 8) & 0x7f);

    size_t sent = 0;
    while (sent < sizeof(packet)) {
        int64_t cur = get_boottime_ms();
        if (cur >= deadline || cur >= overall_deadline_ms) {
            return -2;
        }
        struct pollfd pfd;
        pfd.fd = sock;
        pfd.events = POLLOUT;
        pfd.revents = 0;
        int prc = poll(&pfd, 1, (int)(deadline - cur));
        cur = get_boottime_ms();
        if (cur >= deadline || cur >= overall_deadline_ms) {
            return -2;
        }
        if (prc == 0) {
            return -2;
        }
        if (prc < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        ssize_t n = send(sock, packet + sent, sizeof(packet) - sent, MSG_NOSIGNAL);
        cur = get_boottime_ms();
        if (cur >= deadline || cur >= overall_deadline_ms) {
            return -2;
        }
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (n == 0) {
            return -1;
        }
        sent += (size_t)n;
    }

    int frames_seen = 0;
    bool matched = false;

    while (1) {
        int64_t cur = get_boottime_ms();
        if (cur >= deadline || cur >= overall_deadline_ms) {
            return -2;
        }

        while (*buf_len >= 4) {
            if (buf[0] != 0xff) {
                return -3; /* Malformed header */
            }
            size_t payload_len = buf[1];
            size_t frame_len = 4 + payload_len;
            if (*buf_len < frame_len) {
                break; /* Fragmented remaining frame: preserve */
            }

            frames_seen++;
            if (frames_seen > FRAMES_PER_GETTER_MAX) {
                return -3; /* Frame count limit exceeded */
            }

            uint16_t r_cmd = (uint16_t)(((uint8_t)buf[2] >> 1) | (((uint16_t)(buf[3] & 0x7f)) << 8));
            uint8_t r_kind = (uint8_t)(((buf[2] & 0x01) << 1) | (((uint8_t)buf[3] >> 7) & 0x01));

            if (r_cmd == command && r_kind == 2) {
                if (payload_len != expected_payload_len) {
                    return -3; /* Short or wrong payload length */
                }
                memcpy(out_payload, buf + 4, payload_len);
                matched = true;
            }

            /* Drain complete frame */
            memmove(buf, buf + frame_len, *buf_len - frame_len);
            *buf_len -= frame_len;
        }

        if (*buf_len > 0 && buf[0] != 0xff) {
            return -3; /* Malformed trailing byte */
        }

        if (matched) {
            cur = get_boottime_ms();
            if (cur >= deadline || cur >= overall_deadline_ms) {
                return -2;
            }
            return 0;
        }

        cur = get_boottime_ms();
        if (cur >= deadline || cur >= overall_deadline_ms) {
            return -2;
        }

        struct pollfd pfd;
        pfd.fd = sock;
        pfd.events = POLLIN;
        pfd.revents = 0;
        int prc = poll(&pfd, 1, (int)(deadline - cur));
        cur = get_boottime_ms();
        if (cur >= deadline || cur >= overall_deadline_ms) {
            return -2;
        }
        if (prc == 0) {
            return -2;
        }
        if (prc < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }

        if (*buf_len >= buf_cap) {
            return -3; /* Buffer limit exceeded */
        }

        ssize_t n = recv(sock, buf + *buf_len, buf_cap - *buf_len, 0);
        cur = get_boottime_ms();
        if (cur >= deadline || cur >= overall_deadline_ms) {
            return -2;
        }
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (n == 0) {
            return -3; /* Premature EOF */
        }
        *buf_len += (size_t)n;
    }
}

/* Concise selftest exercising production parser, mappings, and socket handling */
#define SELFTEST_FAIL(msg) do { \
    fprintf(stderr, "selftest failure: %s (line %d)\n", (msg), __LINE__); \
    return 1; \
} while (0)

static int run_selftests(void) {
    /* 1. Address validation & normalization */
    char addr_out[18];
    if (!validate_and_normalize_address("aa:bb:cc:dd:ee:ff", addr_out, sizeof(addr_out)) ||
        strcmp(addr_out, "AA:BB:CC:DD:EE:FF") != 0) {
        SELFTEST_FAIL("address normalization lowercase");
    }
    if (!validate_and_normalize_address("00:11:22:33:44:55", addr_out, sizeof(addr_out)) ||
        strcmp(addr_out, "00:11:22:33:44:55") != 0) {
        SELFTEST_FAIL("address normalization digits");
    }
    if (validate_and_normalize_address("aa:bb:cc:dd:ee", addr_out, sizeof(addr_out)) ||
        validate_and_normalize_address("aa:bb:cc:dd:ee:ff:00", addr_out, sizeof(addr_out)) ||
        validate_and_normalize_address("aa-bb-cc-dd-ee-ff", addr_out, sizeof(addr_out)) ||
        validate_and_normalize_address("gg:bb:cc:dd:ee:ff", addr_out, sizeof(addr_out))) {
        SELFTEST_FAIL("address validation invalid inputs");
    }

    /* 2. Production mapping checks (emit_success pure formatting func) */
    char json_buf[1024];
    const uint8_t anc_fix[1] = {1};                       /* mode 1 -> anc */
    const uint8_t pwr_fix[4] = {0x05, 89, 0xff, 100};     /* modecode 5, left 89, right null, case 100 */
    const uint8_t chg_fix[2] = {0x10, 0x01};             /* mask 0x10 -> left false, right true; case 1 -> true */
    format_success_json(json_buf, sizeof(json_buf), "AA:BB:CC:DD:EE:FF", anc_fix, pwr_fix, chg_fix);
    const char *expected_json =
        "{\"status\":\"ok\",\"address\":\"AA:BB:CC:DD:EE:FF\",\"left\":89,\"right\":null,\"case\":100,"
        "\"mode\":\"anc\",\"left_charging\":false,\"right_charging\":true,\"case_charging\":true}\n";
    if (strcmp(json_buf, expected_json) != 0) {
        SELFTEST_FAIL("expected baseline JSON mismatch");
    }

    /* Battery null/0/100/101 boundary checks */
    const uint8_t pwr_bnd1[4] = {0x05, 0, 100, 101};
    format_success_json(json_buf, sizeof(json_buf), "AA:BB:CC:DD:EE:FF", anc_fix, pwr_bnd1, chg_fix);
    if (!strstr(json_buf, "\"left\":0,\"right\":100,\"case\":null")) {
        SELFTEST_FAIL("battery boundary 0/100/101 mismatch");
    }
    const uint8_t pwr_bnd2[4] = {0x05, 101, 0, 100};
    format_success_json(json_buf, sizeof(json_buf), "AA:BB:CC:DD:EE:FF", anc_fix, pwr_bnd2, chg_fix);
    if (!strstr(json_buf, "\"left\":null,\"right\":0,\"case\":100")) {
        SELFTEST_FAIL("battery boundary 101/0/100 mismatch");
    }
    const uint8_t pwr_bnd3[4] = {0x05, 255, 255, 0};
    format_success_json(json_buf, sizeof(json_buf), "AA:BB:CC:DD:EE:FF", anc_fix, pwr_bnd3, chg_fix);
    if (!strstr(json_buf, "\"left\":null,\"right\":null,\"case\":0")) {
        SELFTEST_FAIL("battery boundary null/null/0 mismatch");
    }

    /* 4 valid charging masks: 0x00, 0x01, 0x10, 0x11 */
    const uint8_t chg_00[2] = {0x00, 0x00};
    format_success_json(json_buf, sizeof(json_buf), "AA:BB:CC:DD:EE:FF", anc_fix, pwr_fix, chg_00);
    if (!strstr(json_buf, "\"left_charging\":false,\"right_charging\":false,\"case_charging\":false")) {
        SELFTEST_FAIL("charging mask 0x00 mismatch");
    }
    const uint8_t chg_01[2] = {0x01, 0x01};
    format_success_json(json_buf, sizeof(json_buf), "AA:BB:CC:DD:EE:FF", anc_fix, pwr_fix, chg_01);
    if (!strstr(json_buf, "\"left_charging\":true,\"right_charging\":false,\"case_charging\":true")) {
        SELFTEST_FAIL("charging mask 0x01 mismatch");
    }
    const uint8_t chg_10[2] = {0x10, 0x00};
    format_success_json(json_buf, sizeof(json_buf), "AA:BB:CC:DD:EE:FF", anc_fix, pwr_fix, chg_10);
    if (!strstr(json_buf, "\"left_charging\":false,\"right_charging\":true,\"case_charging\":false")) {
        SELFTEST_FAIL("charging mask 0x10 mismatch");
    }
    const uint8_t chg_11[2] = {0x11, 0x02};
    format_success_json(json_buf, sizeof(json_buf), "AA:BB:CC:DD:EE:FF", anc_fix, pwr_fix, chg_11);
    if (!strstr(json_buf, "\"left_charging\":true,\"right_charging\":true,\"case_charging\":null")) {
        SELFTEST_FAIL("charging mask 0x11 with case 2 mismatch");
    }

    /* Invalid masks: 0x02, 0x03, 0xff -> unknown (null) */
    const uint8_t bad_masks[3] = {0x02, 0x03, 0xff};
    for (int i = 0; i < 3; i++) {
        const uint8_t chg_bad[2] = {bad_masks[i], 0x00};
        format_success_json(json_buf, sizeof(json_buf), "AA:BB:CC:DD:EE:FF", anc_fix, pwr_fix, chg_bad);
        if (!strstr(json_buf, "\"left_charging\":null,\"right_charging\":null")) {
            SELFTEST_FAIL("invalid charging mask not null");
        }
    }

    /* Modes: 0 -> off, 1 -> anc, 2 -> ambient, other -> unknown */
    const uint8_t anc_off[1] = {0};
    format_success_json(json_buf, sizeof(json_buf), "AA:BB:CC:DD:EE:FF", anc_off, pwr_fix, chg_fix);
    if (!strstr(json_buf, "\"mode\":\"off\"")) SELFTEST_FAIL("mode 0 off mismatch");
    const uint8_t anc_anc[1] = {1};
    format_success_json(json_buf, sizeof(json_buf), "AA:BB:CC:DD:EE:FF", anc_anc, pwr_fix, chg_fix);
    if (!strstr(json_buf, "\"mode\":\"anc\"")) SELFTEST_FAIL("mode 1 anc mismatch");
    const uint8_t anc_amb[1] = {2};
    format_success_json(json_buf, sizeof(json_buf), "AA:BB:CC:DD:EE:FF", anc_amb, pwr_fix, chg_fix);
    if (!strstr(json_buf, "\"mode\":\"ambient\"")) SELFTEST_FAIL("mode 2 ambient mismatch");
    const uint8_t anc_unk1[1] = {3};
    format_success_json(json_buf, sizeof(json_buf), "AA:BB:CC:DD:EE:FF", anc_unk1, pwr_fix, chg_fix);
    if (!strstr(json_buf, "\"mode\":\"unknown\"")) SELFTEST_FAIL("mode 3 unknown mismatch");
    const uint8_t anc_unk2[1] = {255};
    format_success_json(json_buf, sizeof(json_buf), "AA:BB:CC:DD:EE:FF", anc_unk2, pwr_fix, chg_fix);
    if (!strstr(json_buf, "\"mode\":\"unknown\"")) SELFTEST_FAIL("mode 255 unknown mismatch");

    /* 3. Stream framing with AF_UNIX socketpairs and exact wire reference fixtures */
    /* Reference responses: ANC FF01252501, power FF0425070559ff64, charging FF0225081001 */
    /* Expected getter requests: ANC FF002425, power FF002407, charging FF002408 */
    int sv[2];
    if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sv) < 0) {
        SELFTEST_FAIL("socketpair creation failed");
    }
    uint8_t buf[BUFFER_MAX];
    size_t buf_len = 0;
    int64_t dl = get_boottime_ms() + 2000;

    const uint8_t ref_anc[] = {0xff, 0x01, 0x25, 0x25, 0x01};
    const uint8_t ref_pwr[] = {0xff, 0x04, 0x25, 0x07, 0x05, 89, 0xff, 100};
    const uint8_t ref_chg[] = {0xff, 0x02, 0x25, 0x08, 0x10, 0x01};

    const uint8_t req_anc_exp[4] = {0xff, 0x00, 0x24, 0x25};
    const uint8_t req_pwr_exp[4] = {0xff, 0x00, 0x24, 0x07};
    const uint8_t req_chg_exp[4] = {0xff, 0x00, 0x24, 0x08};
    uint8_t req_act[4];

    uint8_t anc_out[1] = {0};
    uint8_t pwr_out[4] = {0};
    uint8_t chg_out[2] = {0};

    if (write(sv[1], ref_anc, sizeof(ref_anc)) != (ssize_t)sizeof(ref_anc) ||
        run_getter(sv[0], 0x2512, 1, anc_out, buf, &buf_len, sizeof(buf), dl) != 0 ||
        anc_out[0] != 1 ||
        read(sv[1], req_act, 4) != 4 ||
        memcmp(req_act, req_anc_exp, 4) != 0) {
        close(sv[0]); close(sv[1]);
        SELFTEST_FAIL("getter 1 (ANC) response/request validation");
    }
    if (write(sv[1], ref_pwr, sizeof(ref_pwr)) != (ssize_t)sizeof(ref_pwr) ||
        run_getter(sv[0], 0x0712, 4, pwr_out, buf, &buf_len, sizeof(buf), dl) != 0 ||
        pwr_out[0] != 5 || pwr_out[1] != 89 || pwr_out[2] != 0xff || pwr_out[3] != 100 ||
        read(sv[1], req_act, 4) != 4 ||
        memcmp(req_act, req_pwr_exp, 4) != 0) {
        close(sv[0]); close(sv[1]);
        SELFTEST_FAIL("getter 2 (power) response/request validation");
    }
    if (write(sv[1], ref_chg, sizeof(ref_chg)) != (ssize_t)sizeof(ref_chg) ||
        run_getter(sv[0], 0x0812, 2, chg_out, buf, &buf_len, sizeof(buf), dl) != 0 ||
        chg_out[0] != 0x10 || chg_out[1] != 1 ||
        read(sv[1], req_act, 4) != 4 ||
        memcmp(req_act, req_chg_exp, 4) != 0) {
        close(sv[0]); close(sv[1]);
        SELFTEST_FAIL("getter 3 (charging) response/request validation");
    }
    close(sv[0]); close(sv[1]);

    /* 4. Short deadline */
    if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sv) < 0) SELFTEST_FAIL("socketpair failed");
    buf_len = 0;
    if (run_getter(sv[0], 0x2512, 1, anc_out, buf, &buf_len, sizeof(buf), get_boottime_ms() - 10) != -2) {
        close(sv[0]); close(sv[1]);
        SELFTEST_FAIL("short deadline did not return -2");
    }
    close(sv[0]); close(sv[1]);

    /* 5. Premature EOF: shutdown peer WR retains peer read half so send succeeds and recv sees EOF */
    if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sv) < 0) SELFTEST_FAIL("socketpair failed");
    buf_len = 0;
    if (shutdown(sv[1], SHUT_WR) < 0) {
        close(sv[0]); close(sv[1]);
        SELFTEST_FAIL("shutdown SHUT_WR failed");
    }
    if (run_getter(sv[0], 0x2512, 1, anc_out, buf, &buf_len, sizeof(buf), dl) != -3) {
        close(sv[0]); close(sv[1]);
        SELFTEST_FAIL("premature EOF did not return -3");
    }
    close(sv[0]); close(sv[1]);

    /* 5b. Peer closed completely before send: send fails with EPIPE -> returns -1 */
    if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sv) < 0) SELFTEST_FAIL("socketpair failed");
    buf_len = 0;
    close(sv[1]);
    if (run_getter(sv[0], 0x2512, 1, anc_out, buf, &buf_len, sizeof(buf), dl) != -1) {
        close(sv[0]);
        SELFTEST_FAIL("peer closed before send did not return -1");
    }
    close(sv[0]);

    /* 6. Genuine fragmentation: preloaded first 2 bytes in production buffer, remaining on socketpair */
    if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sv) < 0) SELFTEST_FAIL("socketpair failed");
    buf[0] = ref_anc[0];
    buf[1] = ref_anc[1];
    buf_len = 2;
    if (write(sv[1], ref_anc + 2, sizeof(ref_anc) - 2) != (ssize_t)(sizeof(ref_anc) - 2)) {
        close(sv[0]); close(sv[1]);
        SELFTEST_FAIL("write remaining bytes for fragmented frame failed");
    }
    if (run_getter(sv[0], 0x2512, 1, anc_out, buf, &buf_len, sizeof(buf), dl) != 0 || anc_out[0] != 1) {
        close(sv[0]); close(sv[1]);
        SELFTEST_FAIL("genuine fragmented frame failed to parse");
    }
    if (read(sv[1], req_act, 4) != 4 || memcmp(req_act, req_anc_exp, 4) != 0) {
        close(sv[0]); close(sv[1]);
        SELFTEST_FAIL("request mismatch in fragmented frame test");
    }
    close(sv[0]); close(sv[1]);

    /* 7. Coalesced + duplicate + preserve fragmented remaining */
    if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sv) < 0) SELFTEST_FAIL("socketpair failed");
    buf_len = 0;
    uint8_t coalesced[sizeof(ref_anc) * 2 + 2];
    memcpy(coalesced, ref_anc, sizeof(ref_anc));
    memcpy(coalesced + sizeof(ref_anc), ref_anc, sizeof(ref_anc));
    coalesced[sizeof(ref_anc) * 2] = 0xff;
    coalesced[sizeof(ref_anc) * 2 + 1] = 0x04;
    if (write(sv[1], coalesced, sizeof(coalesced)) != (ssize_t)sizeof(coalesced)) {
        close(sv[0]); close(sv[1]);
        SELFTEST_FAIL("write coalesced buffer failed");
    }
    if (run_getter(sv[0], 0x2512, 1, anc_out, buf, &buf_len, sizeof(buf), dl) != 0 || anc_out[0] != 1) {
        close(sv[0]); close(sv[1]);
        SELFTEST_FAIL("coalesced getter run failed");
    }
    if (buf_len != 2 || buf[0] != 0xff || buf[1] != 0x04) {
        close(sv[0]); close(sv[1]);
        SELFTEST_FAIL("preserved fragmented remainder mismatch");
    }
    close(sv[0]); close(sv[1]);

    /* 8. Sequential valid power responses with right decimal 91 -> 0xff -> 93 */
    if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sv) < 0) SELFTEST_FAIL("socketpair failed");
    buf_len = 0;
    const uint8_t pwr_seq[3][8] = {
        {0xff, 0x04, 0x25, 0x07, 0x05, 89, 91, 100},
        {0xff, 0x04, 0x25, 0x07, 0x05, 89, 0xff, 100},
        {0xff, 0x04, 0x25, 0x07, 0x05, 89, 93, 100},
    };
    const char *expected_right[3] = {"\"right\":91", "\"right\":null", "\"right\":93"};

    for (int i = 0; i < 3; i++) {
        if (write(sv[1], pwr_seq[i], sizeof(pwr_seq[i])) != (ssize_t)sizeof(pwr_seq[i])) {
            close(sv[0]); close(sv[1]);
            SELFTEST_FAIL("write power sequence failed");
        }
        if (run_getter(sv[0], 0x0712, 4, pwr_out, buf, &buf_len, sizeof(buf), dl) != 0) {
            close(sv[0]); close(sv[1]);
            SELFTEST_FAIL("run_getter in power sequence failed");
        }
        if (read(sv[1], req_act, 4) != 4 || memcmp(req_act, req_pwr_exp, 4) != 0) {
            close(sv[0]); close(sv[1]);
            SELFTEST_FAIL("request mismatch in power sequence test");
        }
        format_success_json(json_buf, sizeof(json_buf), "AA:BB:CC:DD:EE:FF", anc_fix, pwr_out, chg_fix);
        if (!strstr(json_buf, expected_right[i])) {
            close(sv[0]); close(sv[1]);
            SELFTEST_FAIL("power sequence JSON formatted output mismatch");
        }
    }
    close(sv[0]); close(sv[1]);

    /* 9. Malformed header (non-0xff byte) */
    if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sv) < 0) SELFTEST_FAIL("socketpair failed");
    buf_len = 0;
    uint8_t malformed_hdr[] = {0x00, 0x01, 0x25, 0x25, 0x01};
    if (write(sv[1], malformed_hdr, sizeof(malformed_hdr)) != (ssize_t)sizeof(malformed_hdr) ||
        run_getter(sv[0], 0x2512, 1, anc_out, buf, &buf_len, sizeof(buf), dl) != -3) {
        close(sv[0]); close(sv[1]);
        SELFTEST_FAIL("malformed header did not return -3");
    }
    close(sv[0]); close(sv[1]);

    /* 9b. Malformed trailing frame after matching response */
    if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sv) < 0) SELFTEST_FAIL("socketpair failed");
    buf_len = 0;
    uint8_t match_plus_bad[sizeof(ref_anc) + 4];
    memcpy(match_plus_bad, ref_anc, sizeof(ref_anc));
    match_plus_bad[sizeof(ref_anc)] = 0x91;
    match_plus_bad[sizeof(ref_anc) + 1] = 0xff;
    match_plus_bad[sizeof(ref_anc) + 2] = 0x93;
    match_plus_bad[sizeof(ref_anc) + 3] = 0x00;
    if (write(sv[1], match_plus_bad, sizeof(match_plus_bad)) != (ssize_t)sizeof(match_plus_bad) ||
        run_getter(sv[0], 0x2512, 1, anc_out, buf, &buf_len, sizeof(buf), dl) != -3) {
        close(sv[0]); close(sv[1]);
        SELFTEST_FAIL("malformed trailing frame did not return -3");
    }
    close(sv[0]); close(sv[1]);

    /* 10. Frame count limit (framecap > 64) */
    if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sv) < 0) SELFTEST_FAIL("socketpair failed");
    buf_len = 0;
    uint8_t dummy_frame[] = {0xff, 0x00, 0x00, 0x00};
    for (int i = 0; i < 65; i++) {
        if (write(sv[1], dummy_frame, sizeof(dummy_frame)) != (ssize_t)sizeof(dummy_frame)) {
            close(sv[0]); close(sv[1]);
            SELFTEST_FAIL("write dummy frames failed");
        }
    }
    if (run_getter(sv[0], 0x2512, 1, anc_out, buf, &buf_len, sizeof(buf), dl) != -3) {
        close(sv[0]); close(sv[1]);
        SELFTEST_FAIL("frame limit > 64 did not return -3");
    }
    close(sv[0]); close(sv[1]);

    printf("ok\n");
    return 0;
}
#undef SELFTEST_FAIL

int main(int argc, char **argv) {
    signal(SIGPIPE, SIG_IGN);

    pid_t parent_before = getppid();
#if defined(PR_SET_PDEATHSIG)
    if (prctl(PR_SET_PDEATHSIG, SIGTERM) < 0) {
        emit_failure("", "protocol");
        return 1;
    }
#else
    emit_failure("", "protocol");
    return 1;
#endif
    if (parent_before == 1 || getppid() != parent_before) {
        emit_failure("", "protocol");
        return 1;
    }

    if (argc == 2 && strcmp(argv[1], "--selftest") == 0) {
        return run_selftests();
    }

    if (argc != 2) {
        emit_failure("", "protocol");
        return 1;
    }

    char normalized_addr[18];
    if (!validate_and_normalize_address(argv[1], normalized_addr, sizeof(normalized_addr))) {
        emit_failure("", "protocol");
        return 1;
    }

    /* Timeout buffer built before arming timer */
    update_timeout_buf(normalized_addr);

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigalrm_handler;
    sigaction(SIGALRM, &sa, NULL);

    int64_t overall_deadline = get_boottime_ms() + OVERALL_DEADLINE_MS;
    arm_itimer_ms(OVERALL_DEADLINE_MS);

    int lock_fd = acquire_user_lock();
    if (lock_fd < 0) {
        stop_timer_and_block_alrm();
        emit_failure(normalized_addr, "busy");
        return 1;
    }

    if (get_boottime_ms() >= overall_deadline) {
        close(lock_fd);
        stop_timer_and_block_alrm();
        emit_failure(normalized_addr, "timeout");
        return 1;
    }

    bdaddr_t dst_addr;
    if (str2ba(normalized_addr, &dst_addr) < 0) {
        close(lock_fd);
        stop_timer_and_block_alrm();
        emit_failure(normalized_addr, "protocol");
        return 1;
    }

    /* SDP stage: 4s deadline capped by global remaining */
    int64_t now = get_boottime_ms();
    int64_t global_rem = overall_deadline - now;
    if (global_rem <= 0) {
        close(lock_fd);
        stop_timer_and_block_alrm();
        emit_failure(normalized_addr, "timeout");
        return 1;
    }
    int64_t sdp_stage_ms = SDP_DEADLINE_MS;
    if (sdp_stage_ms > global_rem) {
        sdp_stage_ms = global_rem;
    }
    arm_itimer_ms(sdp_stage_ms);

    int channel = -1;
    int sdp_rc = sdp_query_channel(&dst_addr, &channel, overall_deadline, now + sdp_stage_ms);

    /* Restore global remaining timer immediately on SDP completion */
    now = get_boottime_ms();
    global_rem = overall_deadline - now;
    if (global_rem <= 0 || sdp_rc == -2) {
        close(lock_fd);
        stop_timer_and_block_alrm();
        emit_failure(normalized_addr, "timeout");
        return 1;
    }
    arm_itimer_ms(global_rem);

    if (sdp_rc != 0) {
        close(lock_fd);
        stop_timer_and_block_alrm();
        emit_failure(normalized_addr, "discovery");
        return 1;
    }

    int sock = rfcomm_connect_nonblocking(&dst_addr, channel, overall_deadline);
    if (sock < 0) {
        close(lock_fd);
        stop_timer_and_block_alrm();
        if (sock == -2 || get_boottime_ms() >= overall_deadline) {
            emit_failure(normalized_addr, "timeout");
        } else {
            emit_failure(normalized_addr, "connect");
        }
        return 1;
    }

    uint8_t buf[BUFFER_MAX];
    size_t buf_len = 0;
    uint8_t anc_payload[1] = {0};
    uint8_t pwr_payload[4] = {0};
    uint8_t chg_payload[2] = {0};

    int rc = run_getter(sock, 0x2512, 1, anc_payload, buf, &buf_len, sizeof(buf), overall_deadline);
    if (rc != 0) {
        close(sock);
        close(lock_fd);
        stop_timer_and_block_alrm();
        if (rc == -2 || get_boottime_ms() >= overall_deadline) {
            emit_failure(normalized_addr, "timeout");
        } else {
            emit_failure(normalized_addr, "protocol");
        }
        return 1;
    }

    rc = run_getter(sock, 0x0712, 4, pwr_payload, buf, &buf_len, sizeof(buf), overall_deadline);
    if (rc != 0) {
        close(sock);
        close(lock_fd);
        stop_timer_and_block_alrm();
        if (rc == -2 || get_boottime_ms() >= overall_deadline) {
            emit_failure(normalized_addr, "timeout");
        } else {
            emit_failure(normalized_addr, "protocol");
        }
        return 1;
    }

    rc = run_getter(sock, 0x0812, 2, chg_payload, buf, &buf_len, sizeof(buf), overall_deadline);
    if (rc != 0) {
        close(sock);
        close(lock_fd);
        stop_timer_and_block_alrm();
        if (rc == -2 || get_boottime_ms() >= overall_deadline) {
            emit_failure(normalized_addr, "timeout");
        } else {
            emit_failure(normalized_addr, "protocol");
        }
        return 1;
    }

    close(sock);
    close(lock_fd);

    if (get_boottime_ms() >= overall_deadline) {
        stop_timer_and_block_alrm();
        emit_failure(normalized_addr, "timeout");
        return 1;
    }

    /* Block SIGALRM around final single JSON write / stop timer to avoid two frames */
    stop_timer_and_block_alrm();
    emit_success(normalized_addr, anc_payload, pwr_payload, chg_payload);
    return 0;
}
