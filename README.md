# ipt_iftag
In linux operating system has an additional option "tag" for the network interfaces configured to run on ipv4 protocol (sysctl net.ipv4.conf.XXX.tag).
If the system has multiple network interfaces, filtering using the interface names significantly increases the number of rules.
If the network interface to assign the parameter "tag", then the filter can operate on groups of interfaces (tag + mask).

Example:

* vlan10-vlan16 - internal local area networks (tag 1-7)
* vlan20-vlan25 - local networks with restricted access (tag 16-21)
* vlan30 - connect to ISP1 (tag 32)
* vlan31 - connection ISP2 (tag 40)

`iptables -A FORWARD -m iftag --tag iif eq 0/7 -m iftag --tag oif in 32-40 -j ACCEPT`

Enables forwarding of packets from vlan10-vlan16 through any ISP

`iptables -A FORWARD -m iftag --tag iif eq oif / 7 -j ACCEPT`

Enables forwarding of packets within any group of interfaces
