
topdir=(shell pwd)
VPATH = .

include ./Rules.make

SUBDIRS =  ne_common ne_crypt ne_net ne_srvcore ne_app  ne_cliapp  ne_quadtree

all: subdirs

subdirs:
	-mkdir ../lib
	for n in $(SUBDIRS); do $(MAKE) -C $$n || exit 1; done

cleangame:
	rm -f *. 
	for n in $(SUBDIRS); do $(MAKE) -C $$n clean; done

clean:
	rm -f *.o *~
	for n in $(SUBDIRS); do $(MAKE) -C $$n clean; done

checkthem:
	for n in $(SUBDIRS); do $(MAKE) -C $$n checkthem; done

check:
	for n in $(SUBDIRS); do $(MAKE) -C $$n check; done

objs:
	for n in $(SUBDIRS); do $(MAKE) -C $$n objs; done

run:
	./bin/srv_test

runtest:
	./bin/Test

