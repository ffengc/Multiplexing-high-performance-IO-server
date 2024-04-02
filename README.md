# Multiplexing-high-performance-IO-server

**多路转接高性能IO服务器**

Here are three types of high-performance IO servers, implemented through multiplexing. They are select, poll, and epoll, respectively. The code implementation language is C/C++. The three servers inside can be combined with the HTTP server, Web server (multi threaded version, etc.), SystemV and other IO models in the BitCode repository to write servers with high IO performance.

## the readme-pdfs
the readmes detail can be seen in the pdfs.
- [select-readme](./readme-pdfs/select-readme.pdf)
- [poll-readme](./readme-pdfs/poll-readme.pdf)
- [epoll-readme](./readme-pdfs/epoll-readme.pdf)

## The definetion of high-performance-IO

The essence of network communication is IO
IO efficiency issue: Network IO efficiency is very low
Why is network IO inefficient?
Taking reading as an example:

1. When we read/recv, what will read/recv do if there is no data in the underlying buffer? -> Blocking 
2. What happens if there is data in the underlying buffer when we `read/recv`? -> Copy

**So `IO`=`wait`+`data copy`**

**So what is efficient IO? What is inefficient IO?** 

- Inefficient: Unit time, most of the time IO class interfaces are actually waiting!!!
- How to improve the efficiency of IO? Let the proportion of waiting decrease!!!!!

**Five IO models:**

1. Blocking type
2. Non blocking polling
3. Signal driven
4. Multiplexing and Multiplexing 
5. Asynchronous IO

**The fourth method is the most efficient**
Why? Because the waiting time per unit time is very low. If a thread/process wants to participate in IO, we call it synchronous IO.
`IO = wait+copy`, so-called participation actually means either participating in wait, participating in copy, or both at the same time.

### How to perform non-blocking IO?

1. Make IO non blocking. When turned on, you can specify a non blocking interface
2. We use a unified approach for non blocking setting **`fcntl()`;**


Codes can be seen in `./non-block-example`

## select

### What is a select and what is it for?

1. Help users wait for multiple File descriptor at one time
2. When the file sockets are ready, select needs to notify the user which sockets are corresponding to the ready sockets. Then, the user can call interfaces such as recv/recvfrom/read to read them.

### man select

```c
/* According to earlier standards */
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);

void FD_CLR(int fd, fd_set *set);
int  FD_ISSET(int fd, fd_set *set);
void FD_SET(int fd, fd_set *set);
void FD_ZERO(fd_set *set);
```

### readfds

1. When entering: user ->kernel, my bit, bit position, indicates the value of the File descriptor, bit content indicates: whether to care - -- assume 0000 1010 indicates ->only care about the read events on File descriptor 1 and 3, I don't care about the read events of other File descriptor
2. When outputting: Kernel ->User, I am an OS, and multiple fds that you asked me to care about have yielded results. The bit position still indicates the value of the File descriptor, but the content indicates ->is ready 0000 1000->indicates that No. 3 File descriptor is ready! -> Users can directly read descriptor 3 without being blocked!

Tips: Because both the user and the kernel will modify the same bitmap. So fd_ After using this thing once, it needs to be reset!

### build a server with select

- [select-readme](./readme-pdfs/poll-readme.pdf)

**Specific code details can be found in the source code.**

**Writing rules for Select server：**

Need a third-party array to store all legitimate fds.

```c
while(true)
{
    // 1. Traverse the array and update the maximum value
    // 2. Traverse the array and add all the fds that need to be concerned to fd_ Set Bitmap
    // 3. Call select for event detection
  	// 4. Traverse the array, find the ready event, and complete the corresponding redo based on the ready event a. accepter b. recver
}
```

**Extension: What if we want to introduce writing? Let's talk about it when we learn epoll.**

### Advantages and disadvantages of select

**Advantages: (Any multiplexing scheme has these advantages)**

1. High efficiency (compared to before, in multiplexer switching, select is only the younger brother) ->Because he has been working on HandlerEvent
2. Application scenario: There are a large number of links, but only a small number of active ones

**Disadvantages:**

1. In order to maintain third-party arrays, the select server will be filled with traversal operations
2. Reset the select output parameters every time it arrives
3. There is an upper limit to the number of fds that can be managed simultaneously by select!
4. Because almost every parameter is input-output, select will frequently copy parameter data from the user to the kernel and from the kernel to the user. 
5. The encoding is quite complex.

## poll

- [poll-readme](./readme-pdfs/poll-readme.pdf)

### what is poll
Poll is actually just making some improvements on select.
Compared to select, the improvements of poll are as follows:
1. The input and output parameters are separated, so there is no need for a large number of reset operations
2. Poll supervised File descriptor no longer has upper limit


### Advantages and disadvantages of poll
Advantages of poll:
1. High efficiency (compared to multiple processes and threads)
2. There are a large number of links, with only a small number being active, which is more suitable
3. Input and output are separate and do not require extensive resetting
4. Parameter level, no upper limit for managing fd
Disadvantages of poll:
1. It still needs to be traversed, and it is the same as detecting fd readiness at the user level and kernel level
2. It is also difficult to avoid the need for kernel to user copying
3. The code for poll is also relatively complex, but easier than select
In fact, the main drawback is still the first one. In order to solve this drawback, we introduced epoll, which means "enhanced poll". However, in fact, epoll is much better than poll.

## epoll
- [epoll-readme](./readme-pdfs/epoll-readme.pdf)
