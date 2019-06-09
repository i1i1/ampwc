Q = @

all:
	$Q$(MAKE) -C common
	$Q$(MAKE) -C compositor
	$Q$(MAKE) -C client

test:
	$Q$(MAKE) -C compositor test &
	$Qsleep 0.5
	$Q$(MAKE) -C client     test &

clean:
	$Q$(MAKE) -C common     clean
	$Q$(MAKE) -C client     clean
	$Q$(MAKE) -C compositor clean

fullclean:
	$Q$(MAKE) -C common     fullclean
	$Q$(MAKE) -C client     fullclean
	$Q$(MAKE) -C compositor fullclean

.PHONY: all clean fullclean test

