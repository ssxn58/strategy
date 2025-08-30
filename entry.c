// entry.c
// 安装cJSON： sudo apt-get install libcjson-dev
// 编译命令参考：gcc entry.c -o entry -lpthread -lcjson

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>     // sleep
#include <pthread.h>    // 多线程
#include <sys/socket.h> // socket
#include <netinet/in.h> // sockaddr_in
#include <arpa/inet.h>  // inet_addr
#include <cjson/cJSON.h> // 需要安装 cJSON 库

// 日志宏（简化版，你可以用更高级的日志库如 log4c）
#define LOG_INFO(fmt, ...) {fprintf(fp, "[INFO] %s:%d - " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__);fflush(fp);}

// 默认值
#define DEFAULT_PING_MSG "123"
#define DEFAULT_SOCK_PORT 53500

// 全局变量（也可以通过参数传递，这里简化处理）
char *g_ping_msg = DEFAULT_PING_MSG;
int g_sock_port = DEFAULT_SOCK_PORT;
FILE* fp;
const char* log_file = "entry.log";
// UDP Sender 类的 C 实现
typedef struct {
    int sockfd;
    struct sockaddr_in remote_addr;
} UdpSender;

// 初始化 UDP Sender
UdpSender* udp_sender_create(int port) {
    UdpSender *sender = (UdpSender*)malloc(sizeof(UdpSender));
    if (!sender) return NULL;

    sender->sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sender->sockfd < 0) {
        perror("socket creation failed");
        free(sender);
        return NULL;
    }

    // 设置地址
    memset(&sender->remote_addr, 0, sizeof(sender->remote_addr));
    sender->remote_addr.sin_family = AF_INET;
    sender->remote_addr.sin_port = htons(port);
    sender->remote_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // localhost

    return sender;
}

// 发送 UDP 消息
void udp_sender_send(UdpSender *sender, const char *msg) {
    if (!sender || !msg) return;
    int len = strlen(msg);
    int sent = sendto(sender->sockfd, msg, len, 0,
                      (struct sockaddr *)&sender->remote_addr, sizeof(sender->remote_addr));
    if (sent < 0) {
        perror("sendto failed");
    } else {
        // LOG_INFO("Sent UDP message: %s", msg); // 可选打印
    }
}

// 销毁 UDP Sender
void udp_sender_destroy(UdpSender *sender) {
    if (sender) {
        if (sender->sockfd >= 0) close(sender->sockfd);
        free(sender);
    }
}

// 模拟 ping 功能：定期发送 UDP 消息
void* ping_thread_func(void *arg) {
    UdpSender *sender = (UdpSender*)arg;
    if (!sender) return NULL;

    LOG_INFO("Ping thread started. Sending to port %d, msg: '%s'", g_sock_port, g_ping_msg);

    while (1) {
        udp_sender_send(sender, g_ping_msg);
        sleep(3); // 每隔 3 秒发送一次
    }

    return NULL;
}

// 模拟主功能：保持运行
void start_main() {
    LOG_INFO("Main loop started. Press Ctrl+C to exit.");
    while (1) {
        sleep(1);
    }
}

// 解析命令行传入的 JSON 字符串
int parse_json_config(const char *json_str) {
    LOG_INFO("parse_json_config:%s", json_str);
    if (!json_str || strlen(json_str) == 0) {
        LOG_INFO("No JSON config provided, using defaults.");
        return 0;
    }

    cJSON *root = cJSON_Parse(json_str);
    if (!root) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            LOG_INFO("JSON Parse error before: %s", error_ptr);
        }
        return -1;
    }

    // 解析 ping_msg
    cJSON *ping_msg_item = cJSON_GetObjectItem(root, "ping_msg");
    if (ping_msg_item && cJSON_IsString(ping_msg_item)) {
        g_ping_msg = strdup(ping_msg_item->valuestring); // 复制字符串
        LOG_INFO("Using ping_msg from JSON: '%s'", g_ping_msg);
    } else {
        LOG_INFO("No or invalid ping_msg in JSON, using default: '%s'", DEFAULT_PING_MSG);
    }

    // 解析 sock_port
    cJSON *sock_port_item = cJSON_GetObjectItem(root, "sock_port");
    if (sock_port_item && cJSON_IsNumber(sock_port_item)) {
        g_sock_port = sock_port_item->valueint;
        LOG_INFO("Using sock_port from JSON: %d", g_sock_port);
    } else {
        LOG_INFO("No or invalid sock_port in JSON, using default: %d", DEFAULT_SOCK_PORT);
    }

    cJSON_Delete(root);
    return 0;
}

// 程序入口
int main(int argc, char *argv[]) {
    fp = fopen(log_file, "w");
    LOG_INFO("%s starting...", argv[0]);

    if (argc < 2) {
        fprintf(stderr, "Usage: %s '{\"ping_msg\":\"hello\",\"sock_port\":53500}'\n", argv[0]);
        return 1;
    }

    const char *json_input = argv[1];

    // 1. 解析 JSON 配置
    if (parse_json_config(json_input) != 0) {
        LOG_INFO("Failed to parse JSON, using defaults.");
    }

    // 2. 创建 UDP Sender
    UdpSender *sender = udp_sender_create(g_sock_port);
    if (!sender) {
        fprintf(stderr, "Failed to create UDP sender\n");
        return 1;
    }

    // 3. 启动 ping 线程
    pthread_t ping_thread;
    if (pthread_create(&ping_thread, NULL, ping_thread_func, sender) != 0) {
        perror("Failed to create ping thread");
        udp_sender_destroy(sender);
        return 1;
    }

    // 4. 主循环
    start_main();

    // 5. 清理（通常不会执行到这里，因为 start_main() 是无限循环）
    pthread_cancel(ping_thread);      // 尝试取消线程（不一定成功）
    pthread_join(ping_thread, NULL);  // 等待线程结束（可能阻塞）
    udp_sender_destroy(sender);

    if (g_ping_msg != DEFAULT_PING_MSG) {
        free(g_ping_msg);
    }

    LOG_INFO("Program exited.");
    return 0;
}
