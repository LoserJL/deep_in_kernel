cmd_/home/jianglei/deep_in_kernel/data_structure/list/modules.order := {   echo /home/jianglei/deep_in_kernel/data_structure/list/list.ko; :; } | awk '!x[$$0]++' - > /home/jianglei/deep_in_kernel/data_structure/list/modules.order