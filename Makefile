Q = @
MAKE = make --no-print-directory

all:
	$Q$(MAKE) -C common     PREF=common/
	$Q$(MAKE) -C compositor PREF=compositor/
	$Q$(MAKE) -C client     PREF=client/

test:
	$Q$(MAKE) -C compositor test PREF=compositor/ &
	$Qsleep 0.5
	$Q$(MAKE) -C client     test PREF=client/ &

clean:
	$Q$(MAKE) -C common     clean PREF=common/
	$Q$(MAKE) -C client     clean PREF=client/
	$Q$(MAKE) -C compositor clean PREF=compositor/

fullclean:
	$Q$(MAKE) -C common     fullclean PREF=common/
	$Q$(MAKE) -C client     fullclean PREF=client/
	$Q$(MAKE) -C compositor fullclean PREF=compositor/

.PHONY: all clean fullclean test

