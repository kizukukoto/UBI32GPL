#
# Makefile for lcdrouter
#

all: projects_dir

projects_dir: modules_dir
	$(MAKE) -C projects

modules_dir: src_dir
	$(MAKE) -C modules

src_dir: 
	$(MAKE) -C src

romfs:

clean:
	$(MAKE) -C src clean
	$(MAKE) -C modules clean
	$(MAKE) -C projects clean
	rm -rf build

install:
	$(MAKE) -C src install
	$(MAKE) -C modules install
	$(MAKE) -C projects install

	


