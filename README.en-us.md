# winptables (Windows Packet Tables)


### [Official Site](https://winptables.icseclab.org) | [Wiki](https://github.com/icSecLab/winptables/wiki)

[简体中文](./README.md) | English

> Notice:The other language versions document may not be updated in time. When inconsistent, the Chinese version document shall prevail. 

Winptables is also called Windows Packet Tables. A lightweight packet filtering firewall framework for windows platform. It can be used in network packet filtering, routing, NAT, network auditing and others.


## How it works

Winptables consists of two parts, Winptables NDIS Driver and WinptablesService.Winptables NDIS Driver is an NDIS filter driver running at the Ring0 layer, which is responsible for intercepting all outbound and inbound data packets of the host and storing them in the buffer. WinptablesService is a Windows service running at the Ring3 layer, responsible for communicating with the kernel driver, reading the buffer to obtain all network packets, and processing the packets according to the corresponding rules. 

## Developing process

#### Finished

- [x] Open source under Apache License Version 2.0.
- [x] Complete the basic functions that Winptables NDIS Driver needs to implement. 
- [x] Test the stability.

#### To be finished

- [ ] Modules management。
- [ ] Raise the packet forwarding speed.
- [ ] Offer some official modules。

#### Official modules

- [ ] linux iptables compatible layer modules. 
- [ ] NAT modules。

