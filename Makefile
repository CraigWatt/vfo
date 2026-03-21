SERVICE_DIR := services/vfo

.PHONY: default all tests clean clean_tests install uninstall valgrind show_names show_objects show_os

default: all

all:
	$(MAKE) -C $(SERVICE_DIR) all

tests:
	$(MAKE) -C $(SERVICE_DIR) tests

clean:
	$(MAKE) -C $(SERVICE_DIR) clean

clean_tests:
	$(MAKE) -C $(SERVICE_DIR) clean_tests

install:
	$(MAKE) -C $(SERVICE_DIR) install

uninstall:
	$(MAKE) -C $(SERVICE_DIR) uninstall

valgrind:
	$(MAKE) -C $(SERVICE_DIR) valgrind

show_names:
	$(MAKE) -C $(SERVICE_DIR) show_names

show_objects:
	$(MAKE) -C $(SERVICE_DIR) show_objects

show_os:
	$(MAKE) -C $(SERVICE_DIR) show_os
