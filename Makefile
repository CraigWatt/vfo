SERVICE_DIR := services/vfo

.PHONY: default all tests e2e ci clean clean_tests install uninstall valgrind show_names show_objects show_os docs-generate docs-build docs-serve

default: all

all:
	$(MAKE) -C $(SERVICE_DIR) all

tests:
	$(MAKE) -C $(SERVICE_DIR) tests

e2e:
	bash tests/e2e/run_profile_actions_e2e.sh
	bash tests/e2e/run_device_conformance_e2e.sh
	bash tests/e2e/run_dv_metadata_optional_e2e.sh

ci: all tests e2e

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

docs-generate:
	bash infra/scripts/generate-profile-docs.sh

docs-build: docs-generate
	mkdocs build --strict

docs-serve: docs-generate
	mkdocs serve
