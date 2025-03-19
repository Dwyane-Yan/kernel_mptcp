#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/net.h>
#include <linux/in.h>
#include <linux/socket.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <net/sock.h>
#include <linux/inet.h>

#define SERVER_PORT 9999
#define SERVER_IP "127.0.0.1"

static struct socket *sock_server;
static struct task_struct *server_thread;

// 服务端线程函数
static int server_func(void *data) {
    struct socket *newsock;
    int err;
    char *buf;
    struct kvec vec;
    struct msghdr msg;

    buf = kmalloc(1024, GFP_KERNEL);
    if (!buf)
        return -ENOMEM;

    while (!kthread_should_stop()) {
        // 等待客户端连接
        err = kernel_accept(sock_server, &newsock, 0);
        if (err < 0) {
            pr_info("Accept error: %d\n", err);
            break;
        }

        // 接收数据
        vec.iov_base = buf;
        vec.iov_len = 1024;
        memset(&msg, 0, sizeof(msg));
        
        err = kernel_recvmsg(newsock, &msg, &vec, 1, 1024, 0);
        if (err > 0) {
            buf[err] = '\0';
            pr_info("Received: %s\n", buf);
        }

        sock_release(newsock);
    }
    
    kfree(buf);
    return 0;
}

// 客户端发送函数
static void client_send(void) {
    struct socket *sock_client;
    struct sockaddr_in s_addr;
    int err;
    char *msg = "Hello world";
    struct kvec vec;
    struct msghdr msg_hdr;

    // 创建客户端socket
    err = sock_create_kern(&init_net, AF_INET, SOCK_STREAM, IPPROTO_MPTCP, &sock_client);
    
    if (err < 0) {
        pr_err("Client socket create error\n");
        return;
    }

    // 设置服务端地址
    memset(&s_addr, 0, sizeof(s_addr));
    s_addr.sin_family = AF_INET;
    s_addr.sin_port = htons(SERVER_PORT);
    in4_pton(SERVER_IP, -1, (u8 *)&s_addr.sin_addr.s_addr, '\0', NULL);

    // 连接服务端
    err = kernel_connect(sock_client, (struct sockaddr *)&s_addr, sizeof(s_addr), 0);
    if (err < 0) {
        pr_err("Connect error: %d\n", err);
        goto out;
    }

    // 发送数据
    vec.iov_base = msg;
    vec.iov_len = strlen(msg);
    memset(&msg_hdr, 0, sizeof(msg_hdr));
    
    err = kernel_sendmsg(sock_client, &msg_hdr, &vec, 1, vec.iov_len);
    if (err > 0)
        pr_info("Sent %d bytes\n", err);

out:
    sock_release(sock_client);
}

static int __init tcp_module_init(void) {
    struct sockaddr_in s_addr;
    int err;

    // 创建服务端socket
    err = sock_create_kern(&init_net, AF_INET, SOCK_STREAM, IPPROTO_MPTCP, &sock_server);
    if (err < 0) {
        pr_err("Server socket create error\n");
        return err;
    }

    // 绑定地址
    memset(&s_addr, 0, sizeof(s_addr));
    s_addr.sin_family = AF_INET;
    s_addr.sin_port = htons(SERVER_PORT);
    s_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    err = kernel_bind(sock_server, (struct sockaddr *)&s_addr, sizeof(s_addr));
    if (err < 0) {
        pr_err("Bind error: %d\n", err);
        goto out;
    }

    // 开始监听
    err = kernel_listen(sock_server, 5);
    if (err < 0) {
        pr_err("Listen error: %d\n", err);
        goto out;
    }

    // 启动服务端线程
    server_thread = kthread_run(server_func, NULL, "tcp_server");
    if (IS_ERR(server_thread)) {
        err = PTR_ERR(server_thread);
        goto out;
    }

    // 启动客户端发送
    client_send();

    return 0;

out:
    sock_release(sock_server);
    return err;
}

static void __exit tcp_module_exit(void) {
    if (server_thread)
        kthread_stop(server_thread);
    
    if (sock_server)
        sock_release(sock_server);
    
    pr_info("TCP module unloaded\n");
}

module_init(tcp_module_init);
module_exit(tcp_module_exit);
MODULE_LICENSE("GPL");

