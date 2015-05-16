all:
	$(MAKE) -C iptables
	$(MAKE) -C kernel

modules_install:
	$(MAKE) -C kernel modules_install
install:
	$(MAKE) -C iptables install
clean:
	$(MAKE) -C kernel clean
	$(MAKE) -C iptables clean
