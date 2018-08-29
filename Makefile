test: stests ltests

stests:
	$(MAKE) -C tests/small-tests

ltests:
	$(MAKE) -C tests/large-tests
