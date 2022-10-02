##### abstract  

intel hypervisor 



##### 分支minivt：

目的是只成功vmlaunch（无多余功能，vm进入后会立刻int3，步过之后无法运行，没有实现vm-exit的处理）。



##### 分支minivt_can_success_run_system:

目的是成功vmlaunch,并且能够最小功能正常运行系统

