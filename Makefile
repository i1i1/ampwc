all:
	make -C common
	make -C compositor
	make -C client

test:
	make -C compositor test
	make -C client     test

clean:
	make -C common     clean
	make -C client     clean
	make -C compositor clean

fullclean:
	make -C common     fullclean
	make -C client     fullclean
	make -C compositor fullclean

.PHONY: all clean fullclean test

