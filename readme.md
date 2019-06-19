# 服务器压测小工具


- 此工具用来模拟多个客户端对服务器发起连接并发送数据,对服务器进行压力测试.在服务器端可以观察其内存CPU等资源使用情况.     
- 目前只实现发送TCP/IP协议请求.    
- 实现方式采取简单的多线程阻塞式socket.每个线程每隔3s发送一次数据,共发送3次(可自行在代码中调整).   


感谢使用! 

<iframe height=600 width=711 src="https://github.com/VVZzzz/ServerTestTool/blob/master/GIF.gif">      
