这个实验因为时间不充足和水平有限，写出来的raft开始本身架构设计不是很完美，代码结构不是很清晰，之后有时间再改吧。

给客户端默认要监听8888端口才能获取结果，自己测试了几个案例，GET,SET,DEL均返回了正确结果，leader挂了之后也能重新选举正常运行，但未测试复杂案例，感觉bug不少，之后有时间再好好改改。

并且在启动之处虽然raft每个成员都有一段睡眠时间，但客户端可以发送命令，会激活raft进行选举leader，然后返回结果。