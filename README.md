## UDPTUN

Simple UDP Tunnel program.

### Usage

#### Client:

``` shell
sudo ./client.elf -s <server_ip>
```

After client up and running, route packet into the network interface called `clienttun` which created by this program. For example:

``` shell
route add default dev clienttun # this will route all your traffic into clienttun
route add 123.123.123.123 clienttun # or like this, route traffic of a specify ip (123.123.123.123)
route add <server_ip> gw <your_gateway_ip> dev <your_original_eth_dev> # use `route -n` to see your gateway
```

#### Server:

``` shell
sudo ./server.elf
```

Server will create a tun named `servertun`. If you'd like to watch the traffic:

``` shell
tcpdump -i servertun -vv -n
```

### FYI

This is a learning purpose program. May contains bug.

Associate blog about this program: [cnblogs:Make a simple udp-tunnel program](http://www.cnblogs.com/blumia/p/Make-a-simple-udp-tunnel-program.html) (Chinese).