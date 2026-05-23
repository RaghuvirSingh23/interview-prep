# Networking Fundamentals - Interview Preparation Guide

## Table of Contents
1. [OSI Model](#osi-model)
2. [TCP/IP Model](#tcpip-model)
3. [IP Addressing](#ip-addressing)
4. [TCP vs UDP](#tcp-vs-udp)
5. [TCP Deep Dive](#tcp-deep-dive)
6. [HTTP/HTTPS](#httphttps)
7. [DNS](#dns)
8. [Network Security](#network-security)
9. [Socket Programming](#socket-programming)
10. [Common Interview Questions](#common-interview-questions)

---

## OSI Model

### 7 Layers (Top to Bottom)

```
+-------------------+------------------+------------------------+
| Layer             | Protocol Data    | Examples               |
|                   | Unit (PDU)       |                        |
+-------------------+------------------+------------------------+
| 7. Application    | Data             | HTTP, FTP, SMTP, DNS   |
| 6. Presentation   | Data             | SSL/TLS, JPEG, ASCII   |
| 5. Session        | Data             | NetBIOS, RPC           |
| 4. Transport      | Segment/Datagram | TCP, UDP               |
| 3. Network        | Packet           | IP, ICMP, ARP          |
| 2. Data Link      | Frame            | Ethernet, WiFi, PPP    |
| 1. Physical       | Bits             | Cables, Hubs, NICs     |
+-------------------+------------------+------------------------+
```

### Mnemonic
**A**ll **P**eople **S**eem **T**o **N**eed **D**ata **P**rocessing
(Application → Physical)

**P**lease **D**o **N**ot **T**hrow **S**ausage **P**izza **A**way
(Physical → Application)

### Layer Functions

**Layer 7 - Application**: User interface, application protocols
**Layer 6 - Presentation**: Data format, encryption, compression
**Layer 5 - Session**: Connection management, synchronization
**Layer 4 - Transport**: End-to-end delivery, flow control, error recovery
**Layer 3 - Network**: Routing, logical addressing (IP)
**Layer 2 - Data Link**: Physical addressing (MAC), error detection
**Layer 1 - Physical**: Bit transmission over physical medium

---

## TCP/IP Model

### 4 Layers

```
+-------------------+------------------------+
| TCP/IP Layer      | OSI Equivalent         |
+-------------------+------------------------+
| Application       | Application +          |
|                   | Presentation + Session |
+-------------------+------------------------+
| Transport         | Transport              |
+-------------------+------------------------+
| Internet          | Network                |
+-------------------+------------------------+
| Network Access    | Data Link + Physical   |
+-------------------+------------------------+
```

### Encapsulation

```
Application Data
       ↓
+------------------+------+
| TCP/UDP Header   | Data |  ← Segment
+------------------+------+
       ↓
+------------+------------------+------+
| IP Header  | TCP/UDP Header   | Data |  ← Packet
+------------+------------------+------+
       ↓
+---------------+------------+------------------+------+-----+
| Frame Header  | IP Header  | TCP/UDP Header   | Data | FCS |  ← Frame
+---------------+------------+------------------+------+-----+
```

---

## IP Addressing

### IPv4 Address

```
32 bits = 4 octets
Example: 192.168.1.100

Binary: 11000000.10101000.00000001.01100100

Classes (historical):
Class A: 1.0.0.0   - 126.255.255.255  (/8)
Class B: 128.0.0.0 - 191.255.255.255  (/16)
Class C: 192.0.0.0 - 223.255.255.255  (/24)
Class D: 224.0.0.0 - 239.255.255.255  (Multicast)
Class E: 240.0.0.0 - 255.255.255.255  (Reserved)
```

### Private IP Ranges

```
10.0.0.0    - 10.255.255.255    (10.0.0.0/8)
172.16.0.0  - 172.31.255.255    (172.16.0.0/12)
192.168.0.0 - 192.168.255.255   (192.168.0.0/16)
```

### Subnetting

```
IP: 192.168.1.100
Subnet Mask: 255.255.255.0 (/24)

Network Address: 192.168.1.0
Broadcast Address: 192.168.1.255
Usable Hosts: 192.168.1.1 - 192.168.1.254 (254 hosts)

CIDR Notation:
/24 = 256 addresses (254 usable)
/25 = 128 addresses (126 usable)
/26 = 64 addresses (62 usable)
/27 = 32 addresses (30 usable)
/28 = 16 addresses (14 usable)
```

### IPv6 Address

```
128 bits = 8 groups of 16 bits (hex)
Example: 2001:0db8:85a3:0000:0000:8a2e:0370:7334

Shortened: 2001:db8:85a3::8a2e:370:7334
(Leading zeros removed, consecutive zeros = ::)

Special Addresses:
::1         = Loopback (like 127.0.0.1)
::          = Unspecified
fe80::/10   = Link-local
```

### NAT (Network Address Translation)

```
Private Network          NAT Router          Internet
+--------+              +--------+          +--------+
| 192.168|              |        |          |        |
| .1.100 |------------->| Public |--------->| Server |
+--------+              | IP     |          |        |
                        +--------+          +--------+

NAT Table:
Internal IP:Port    External IP:Port    Destination
192.168.1.100:5000  203.0.113.1:40000   8.8.8.8:80
```

---

## TCP vs UDP

### Comparison

| Feature | TCP | UDP |
|---------|-----|-----|
| Connection | Connection-oriented | Connectionless |
| Reliability | Guaranteed delivery | Best effort |
| Ordering | Maintains order | No ordering |
| Error Checking | Yes + recovery | Checksum only |
| Flow Control | Yes (sliding window) | No |
| Congestion Control | Yes | No |
| Speed | Slower | Faster |
| Header Size | 20-60 bytes | 8 bytes |
| Use Cases | HTTP, FTP, Email | DNS, Video, Gaming |

### TCP Header

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|          Source Port          |       Destination Port        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                        Sequence Number                        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                    Acknowledgment Number                      |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|  Data |       |U|A|P|R|S|F|                                   |
| Offset| Rsrvd |R|C|S|S|Y|I|            Window                 |
|       |       |G|K|H|T|N|N|                                   |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|           Checksum            |         Urgent Pointer        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                    Options (if any)                           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

### UDP Header

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|          Source Port          |       Destination Port        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|            Length             |           Checksum            |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

---

## TCP Deep Dive

### Three-Way Handshake (Connection Establishment)

```
Client                                Server
   |                                     |
   |  -------- SYN (seq=x) --------->    |
   |                                     |
   |  <---- SYN-ACK (seq=y, ack=x+1) --- |
   |                                     |
   |  -------- ACK (ack=y+1) -------->   |
   |                                     |
   |        Connection Established       |
```

### Four-Way Handshake (Connection Termination)

```
Client                                Server
   |                                     |
   |  -------- FIN (seq=x) --------->    |
   |                                     |
   |  <-------- ACK (ack=x+1) --------   |
   |                                     |
   |  <-------- FIN (seq=y) ----------   |
   |                                     |
   |  -------- ACK (ack=y+1) -------->   |
   |                                     |
   |        Connection Closed            |
```

### TCP States

```
                              +---------+
                              |  CLOSED |
                              +---------+
                                   |
                    passive open   |   active open
                                   |   send SYN
                              +---------+
                              |  LISTEN |
                              +---------+
                                   |
                    rcv SYN        |
                    send SYN,ACK   |
                              +---------+
                              | SYN_RCVD|
                              +---------+
                                   |
                    rcv ACK        |
                                   |
                              +---------+
                              |  ESTAB  |
                              +---------+
```

### Flow Control (Sliding Window)

```
Sender Window (size = 4):
+---+---+---+---+---+---+---+---+---+---+
| 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 |10 |
+---+---+---+---+---+---+---+---+---+---+
      ^               ^
      |               |
   Sent but        Can send
   unacked         next

Receiver advertises window size in ACK
Sender adjusts sending rate accordingly
```

### Congestion Control

**Slow Start**: Start with small window, double each RTT
**Congestion Avoidance**: Linear increase after threshold
**Fast Retransmit**: Retransmit on 3 duplicate ACKs
**Fast Recovery**: Don't go back to slow start on dup ACKs

```
Window Size
    ^
    |        /\
    |       /  \
    |      /    \___/\___
    |     /              \
    |    /                \
    |   /                  \
    |  /
    | /
    |/
    +-------------------------> Time
      Slow   Congestion    Fast
      Start  Avoidance     Recovery
```

---

## HTTP/HTTPS

### HTTP Methods

| Method | Description | Idempotent | Safe |
|--------|-------------|------------|------|
| GET | Retrieve resource | Yes | Yes |
| POST | Create resource | No | No |
| PUT | Update/Replace | Yes | No |
| PATCH | Partial update | No | No |
| DELETE | Delete resource | Yes | No |
| HEAD | GET without body | Yes | Yes |
| OPTIONS | Supported methods | Yes | Yes |

### HTTP Status Codes

```
1xx - Informational
  100 Continue
  101 Switching Protocols

2xx - Success
  200 OK
  201 Created
  204 No Content

3xx - Redirection
  301 Moved Permanently
  302 Found (Temporary)
  304 Not Modified

4xx - Client Error
  400 Bad Request
  401 Unauthorized
  403 Forbidden
  404 Not Found
  405 Method Not Allowed
  429 Too Many Requests

5xx - Server Error
  500 Internal Server Error
  502 Bad Gateway
  503 Service Unavailable
  504 Gateway Timeout
```

### HTTP/1.1 vs HTTP/2 vs HTTP/3

| Feature | HTTP/1.1 | HTTP/2 | HTTP/3 |
|---------|----------|--------|--------|
| Protocol | TCP | TCP | QUIC (UDP) |
| Connections | Multiple | Single (multiplexed) | Single |
| Header Compression | No | HPACK | QPACK |
| Server Push | No | Yes | Yes |
| Head-of-line Blocking | Yes | Stream level | No |

### HTTPS (TLS Handshake)

```
Client                                  Server
   |                                       |
   |  ------ ClientHello ---------------> |
   |        (supported ciphers, random)   |
   |                                       |
   |  <----- ServerHello ----------------- |
   |        (chosen cipher, random)        |
   |  <----- Certificate ----------------- |
   |  <----- ServerHelloDone ------------- |
   |                                       |
   |  ------ ClientKeyExchange ---------> |
   |        (pre-master secret)            |
   |  ------ ChangeCipherSpec ----------> |
   |  ------ Finished ------------------> |
   |                                       |
   |  <----- ChangeCipherSpec ------------ |
   |  <----- Finished -------------------- |
   |                                       |
   |        Encrypted Communication        |
```

### Common HTTP Headers

```
Request Headers:
Host: example.com
User-Agent: Mozilla/5.0
Accept: text/html, application/json
Accept-Language: en-US
Accept-Encoding: gzip, deflate
Authorization: Bearer <token>
Cookie: session=abc123
Content-Type: application/json
Content-Length: 1234

Response Headers:
Content-Type: application/json
Content-Length: 5678
Cache-Control: max-age=3600
Set-Cookie: session=xyz789
Location: /new-url (for redirects)
Access-Control-Allow-Origin: *
```

---

## DNS

### DNS Hierarchy

```
                    . (Root)
                      |
        +-------------+-------------+
        |             |             |
       com           org           net
        |
    +---+---+
    |       |
  google  amazon
    |
   www
```

### DNS Record Types

| Type | Description | Example |
|------|-------------|---------|
| A | IPv4 address | example.com → 93.184.216.34 |
| AAAA | IPv6 address | example.com → 2606:2800:220:1:... |
| CNAME | Canonical name (alias) | www → example.com |
| MX | Mail exchanger | example.com → mail.example.com |
| NS | Name server | example.com → ns1.example.com |
| TXT | Text record | SPF, DKIM verification |
| PTR | Reverse lookup | IP → domain |
| SOA | Start of Authority | Zone metadata |

### DNS Resolution Process

```
1. User types www.example.com

2. Check browser cache
   ↓ (miss)
3. Check OS cache (/etc/hosts)
   ↓ (miss)
4. Query Recursive Resolver (ISP DNS)
   ↓ (miss)
5. Query Root Server (.)
   → Returns .com NS
   ↓
6. Query TLD Server (.com)
   → Returns example.com NS
   ↓
7. Query Authoritative Server (example.com)
   → Returns IP address
   ↓
8. Cache result, return to user
```

### DNS Query Types

**Recursive**: Resolver does all work, returns final answer
**Iterative**: Resolver returns referral, client follows up

---

## Network Security

### Common Attacks

**DDoS (Distributed Denial of Service)**:
- Flood target with traffic
- Types: Volumetric, Protocol, Application layer
- Mitigation: Rate limiting, CDN, traffic scrubbing

**Man-in-the-Middle (MITM)**:
- Attacker intercepts communication
- Can read/modify data
- Prevention: TLS/HTTPS, certificate pinning

**ARP Spoofing**:
- Fake ARP responses to redirect traffic
- Prevention: Static ARP entries, ARP inspection

**DNS Spoofing**:
- Return fake DNS responses
- Prevention: DNSSEC, DNS over HTTPS (DoH)

**SYN Flood**:
- Send many SYN packets, never complete handshake
- Exhausts server connection table
- Prevention: SYN cookies, rate limiting

### Firewalls

**Packet Filter**: Check IP/port, allow/deny
**Stateful**: Track connection state
**Application Layer**: Inspect application data (WAF)

```
iptables examples:

# Allow incoming SSH
iptables -A INPUT -p tcp --dport 22 -j ACCEPT

# Block IP
iptables -A INPUT -s 192.168.1.100 -j DROP

# Allow established connections
iptables -A INPUT -m state --state ESTABLISHED,RELATED -j ACCEPT
```

### VPN (Virtual Private Network)

```
+--------+          +--------+          +--------+
| Client |--tunnel--| VPN    |--regular--| Server |
|        |  (enc)   | Server |  traffic  |        |
+--------+          +--------+          +--------+

Types:
- Site-to-Site: Connect networks
- Remote Access: Individual users
- Protocols: OpenVPN, WireGuard, IPSec
```

---

## Socket Programming

### TCP Server (C)

```c
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

int main() {
    // 1. Create socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    // 2. Set socket options (reuse address)
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // 3. Bind to address
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(8080)
    };
    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    
    // 4. Listen for connections
    listen(server_fd, 5);  // backlog = 5
    
    printf("Server listening on port 8080...\n");
    
    // 5. Accept connection
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
    
    // 6. Read/Write
    char buffer[1024] = {0};
    read(client_fd, buffer, sizeof(buffer));
    printf("Received: %s\n", buffer);
    
    char *response = "Hello from server!";
    write(client_fd, response, strlen(response));
    
    // 7. Close
    close(client_fd);
    close(server_fd);
    
    return 0;
}
```

### TCP Client (C)

```c
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

int main() {
    // 1. Create socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    
    // 2. Connect to server
    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(8080)
    };
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
    
    connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    
    // 3. Send data
    char *message = "Hello from client!";
    send(sock, message, strlen(message), 0);
    
    // 4. Receive response
    char buffer[1024] = {0};
    recv(sock, buffer, sizeof(buffer), 0);
    printf("Server response: %s\n", buffer);
    
    // 5. Close
    close(sock);
    
    return 0;
}
```

### UDP Server/Client (C)

```c
// UDP Server
int server_fd = socket(AF_INET, SOCK_DGRAM, 0);

struct sockaddr_in addr = {
    .sin_family = AF_INET,
    .sin_addr.s_addr = INADDR_ANY,
    .sin_port = htons(8080)
};
bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));

char buffer[1024];
struct sockaddr_in client_addr;
socklen_t client_len = sizeof(client_addr);

// Receive (no accept needed!)
recvfrom(server_fd, buffer, sizeof(buffer), 0,
         (struct sockaddr *)&client_addr, &client_len);

// Send response
sendto(server_fd, "Response", 8, 0,
       (struct sockaddr *)&client_addr, client_len);

// UDP Client
int sock = socket(AF_INET, SOCK_DGRAM, 0);

struct sockaddr_in server_addr = {
    .sin_family = AF_INET,
    .sin_port = htons(8080)
};
inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

// No connect needed (but can use for convenience)
sendto(sock, "Hello", 5, 0,
       (struct sockaddr *)&server_addr, sizeof(server_addr));

recvfrom(sock, buffer, sizeof(buffer), 0, NULL, NULL);
```

### select() for Multiple Connections

```c
fd_set read_fds;
int max_fd = server_fd;

while (1) {
    FD_ZERO(&read_fds);
    FD_SET(server_fd, &read_fds);
    
    for (int i = 0; i < num_clients; i++) {
        FD_SET(client_fds[i], &read_fds);
        if (client_fds[i] > max_fd) max_fd = client_fds[i];
    }
    
    // Wait for activity
    select(max_fd + 1, &read_fds, NULL, NULL, NULL);
    
    // Check for new connection
    if (FD_ISSET(server_fd, &read_fds)) {
        int new_client = accept(server_fd, ...);
        // Add to client_fds array
    }
    
    // Check existing clients
    for (int i = 0; i < num_clients; i++) {
        if (FD_ISSET(client_fds[i], &read_fds)) {
            // Handle client data
        }
    }
}
```

---

## Common Interview Questions

### Q1: What happens when you type google.com in browser?

1. **DNS Resolution**: Browser → DNS → IP address
2. **TCP Connection**: Three-way handshake with server
3. **TLS Handshake**: If HTTPS, establish encrypted connection
4. **HTTP Request**: Send GET request for /
5. **Server Processing**: Server generates response
6. **HTTP Response**: HTML content returned
7. **Rendering**: Browser parses HTML, requests CSS/JS/images
8. **Display**: Page rendered to user

### Q2: Explain TCP three-way handshake

1. **SYN**: Client sends SYN with initial sequence number
2. **SYN-ACK**: Server responds with SYN-ACK, acknowledges client's seq
3. **ACK**: Client acknowledges server's sequence number
4. Connection established, both sides can send data

**Why 3 steps?** Both sides need to agree on initial sequence numbers

### Q3: Difference between TCP and UDP?

| TCP | UDP |
|-----|-----|
| Connection-oriented | Connectionless |
| Reliable (ACKs, retransmission) | Unreliable |
| Ordered delivery | No ordering |
| Flow control | No flow control |
| Slower | Faster |
| HTTP, FTP, SSH | DNS, Video streaming, Gaming |

### Q4: What is the difference between HTTP and HTTPS?

| HTTP | HTTPS |
|------|-------|
| Port 80 | Port 443 |
| Plain text | Encrypted (TLS) |
| No authentication | Server authenticated via certificate |
| Can be intercepted | Secure against MITM |

### Q5: Explain how DNS works

1. Browser checks cache
2. OS checks /etc/hosts and cache
3. Query recursive resolver (ISP)
4. Resolver queries root servers (.)
5. Root returns TLD server (.com)
6. TLD returns authoritative server
7. Authoritative returns IP address
8. Result cached at each level

### Q6: What is NAT and why is it used?

**NAT** translates private IPs to public IPs
- Conserves IPv4 addresses
- Provides security (hides internal network)
- Allows multiple devices to share one public IP

### Q7: Explain the OSI model layers

1. **Physical**: Bits, cables, signals
2. **Data Link**: Frames, MAC addresses, switches
3. **Network**: Packets, IP addresses, routing
4. **Transport**: Segments, TCP/UDP, ports
5. **Session**: Connection management
6. **Presentation**: Encryption, compression
7. **Application**: HTTP, FTP, user interface

### Q8: What is a subnet mask?

Divides IP into network and host portions:
- 255.255.255.0 (/24): First 24 bits = network, last 8 = host
- Determines which IPs are on same network
- Used for routing decisions

### Q9: How does ARP work?

**ARP** (Address Resolution Protocol) maps IP to MAC:
1. Device needs MAC for known IP
2. Broadcasts ARP request: "Who has 192.168.1.1?"
3. Target responds: "192.168.1.1 is at AA:BB:CC:DD:EE:FF"
4. Requester caches result

### Q10: What is the difference between a hub, switch, and router?

| Device | Layer | Function |
|--------|-------|----------|
| Hub | 1 (Physical) | Broadcasts to all ports |
| Switch | 2 (Data Link) | Forwards based on MAC address |
| Router | 3 (Network) | Forwards based on IP address |

---

## Quick Reference

### Common Ports

| Port | Service |
|------|---------|
| 20, 21 | FTP |
| 22 | SSH |
| 23 | Telnet |
| 25 | SMTP |
| 53 | DNS |
| 67, 68 | DHCP |
| 80 | HTTP |
| 110 | POP3 |
| 143 | IMAP |
| 443 | HTTPS |
| 3306 | MySQL |
| 5432 | PostgreSQL |
| 6379 | Redis |
| 27017 | MongoDB |

### Network Commands

```bash
# IP configuration
ip addr              # Linux
ifconfig             # macOS/older Linux
ipconfig             # Windows

# DNS lookup
nslookup google.com
dig google.com
host google.com

# Connectivity
ping google.com
traceroute google.com  # Linux/macOS
tracert google.com     # Windows

# Network statistics
netstat -an           # All connections
ss -tuln              # Linux (faster than netstat)

# ARP table
arp -a

# Routing table
route -n              # Linux
netstat -rn           # macOS

# Packet capture
tcpdump -i eth0
wireshark

# Port scanning
nmap -p 1-1000 target

# HTTP requests
curl -v https://example.com
wget https://example.com
```

### Subnet Cheat Sheet

| CIDR | Subnet Mask | Hosts |
|------|-------------|-------|
| /32 | 255.255.255.255 | 1 |
| /30 | 255.255.255.252 | 2 |
| /28 | 255.255.255.240 | 14 |
| /27 | 255.255.255.224 | 30 |
| /26 | 255.255.255.192 | 62 |
| /25 | 255.255.255.128 | 126 |
| /24 | 255.255.255.0 | 254 |
| /23 | 255.255.254.0 | 510 |
| /22 | 255.255.252.0 | 1022 |
| /16 | 255.255.0.0 | 65534 |
| /8 | 255.0.0.0 | 16777214 |
