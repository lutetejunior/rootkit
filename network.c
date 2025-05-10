#include <linux/module.h>
#include <linux/net.h>
#include <linux/inet.h>
#include <linux/kthread.h>

#define BUF_SIZE 1025

static struct task_struct *thread_client = NULL;
static struct socket *sock = NULL;
static char *ip = "127.0.0.1";
static int port = 4242;
static char *greet_message = "Hello World! from kernel land\n";
module_param(ip, charp, 0644);
module_param(port, int, 0644);
module_param(greet_message, charp, 0644);
MODULE_PARM_DESC(ip, "Server IPv4");
MODULE_PARM_DESC(port, "Server port");
MODULE_PARM_DESC(greet_message, "Greeting message to send to the server");

static void *convert(void *ptr)
{
  return ptr;
}

static void to_upper(char *str, int len)
{
  for (int i = 0; i < len; ++i)
    if ('a' <= str[i] && str[i] <= 'z')
      str[i] -= 32;
}

static int client(void *data)
{
  struct sockaddr_in addr = {0};
  struct msghdr msg = {0};
  struct msghdr rmsg = {0};
  struct kvec vec = {0};
  struct kvec rvec = {0};
  char *kt_data = data;
  char buf[BUF_SIZE] = {0};
  unsigned char ip_binary[4] = {0};
  int ret = 0;

  pr_info("network: thread: %s with data %s\n", thread_client->comm, kt_data);
  if ((ret = in4_pton(ip, -1, ip_binary, -1, NULL)) == 0)
  {
    pr_err("network: error converting the IPv4 address: %d\n", ret);
    return 1;
  }

  while (kthread_should_stop() == 0)
  {

    if ((ret = sock_create(AF_INET, SOCK_STREAM, IPPROTO_TCP, &sock)) < 0)
    {
      pr_err("network: error creating the socket: %d\n", ret);
      continue;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    memcpy(&addr.sin_addr.s_addr, ip_binary, sizeof(addr.sin_addr.s_addr));

    if ((ret = sock->ops->connect(sock, convert(&addr), sizeof(addr), 0)) < 0)
    {
      pr_err("network: error connecting to %s:%d (%d)\n", ip, port, ret);
      sock_release(sock);
      sock = NULL;
      continue;
    }

    vec.iov_base = greet_message;
    vec.iov_len = strlen(greet_message);

    if ((ret = kernel_sendmsg(sock, &msg, &vec, 1, vec.iov_len)) < 0)
    {
      pr_err("network: error sending the greeting message: %d\n", ret);
      sock_release(sock);
      sock = NULL;
      continue;
    }

    pr_info("network: greeting message '%s' sended to %s:%d\n", greet_message, ip, port);

    while (kthread_should_stop() == 0)
    {
      rvec.iov_base = buf;
      rvec.iov_len = BUF_SIZE - 1;

      if ((ret = kernel_recvmsg(sock, &rmsg, &rvec, 1, BUF_SIZE - 1, 0)) <= 0)
      {
        pr_warn("network: connection lost or closed\n");
        sock_release(sock);
        sock = NULL;
        break;
      }

      buf[ret] = '\0';
      pr_info("network: received: %s\n", buf);
      to_upper(buf, ret);
      pr_info("network: sending: %s\n", buf);
      vec.iov_base = buf;
      vec.iov_len = strlen(buf);

      if ((ret = kernel_sendmsg(sock, &msg, &vec, 1, vec.iov_len)) < 0)
      {
        pr_err("network: error sending the greeting message: %d\n", ret);
        sock_release(sock);
        sock = NULL;
        break;
      }
    }
  }
  return 0;
}

static __init int network_init(void)
{
  thread_client = kthread_run(client, NULL, "client");

  pr_info("network: insmoded\n");

  if (IS_ERR(thread_client))
  {
    pr_err("network: failed to create a kthread\n");
    return PTR_ERR(thread_client);
  }

  pr_info("network: thread started\n");
  return 0;
}

static void __exit network_exit(void)
{
  if (thread_client)
  {
    kthread_stop(thread_client);
    pr_info("network: thread stopped\n");
  }

  if (sock)
    sock_release(sock);

  pr_info("network: rmmoded\n");
}

module_init(network_init);
module_exit(network_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Gelules");
MODULE_DESCRIPTION("Connect to a TCP server using ipv4 and STR_TO_UPPER the messages received");