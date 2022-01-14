# winptables (Windows Packet Tables)


### [项目官网](https://winptables.icseclab.org) | [使用帮助](https://github.com/icSecLab/winptables/wiki)

简体中文 | [English](./README.en-us.md)

> 注:其他语言版本的文档可能存在更新不及时的情况，当出现不一致时，以中文版文档为准。

> Notice:The other language versions document may not be updated in time. When inconsistent, the Chinese version document shall prevail. 

Winptables全称Windows Packet Tables，是一款面向windows平台打造的轻量级包过滤防火墙框架。可以通过本框架轻松的实现网络数据包过滤，路由转发，NAT，网络审计等多种功能。


## 工作原理

 Winptables由两部分组成，分别是Winptables NDIS Driver和WinptablesService。其中Winptables NDIS Driver是运行在Ring0层的NDIS过滤驱动程序，负责拦截主机出站和入站的全部数据包并存入缓冲区。WinptablesService是运行在Ring3层的Windows服务，负责与内核驱动通信，读取缓冲区获得所有网络数据包，并根据相应规则对数据包进行处理。

## 开发进度

#### 已完成

- [x] 在Apache License Version 2.0许可下开源。
- [x] 完成NDIS过滤驱动程序需要实现的基本功能。
- [x] 测试WinptablesService和Winptables NDIS Driver基本功能的稳定性。

#### 待开发

- [ ] 实现插件管理功能，制定数据包处理插件开发规范。
- [ ] 提高winptables的包转发效率。
- [ ] 提供常用功能的官方插件。

#### 官方插件（会动态更新添加）

- [ ] 待开发：linux iptables兼容层模块。
- [ ] 待开发：NAT转发模块。

