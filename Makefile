PORT ?= /dev/ttyUSB0
FW ?= micropython-esp32.bin

.PHONY: test flash deploy repl clean
test:
	python3 -m pytest -q
flash:
	bash tools/flash.sh $(PORT) $(FW)
deploy:
	bash tools/deploy.sh $(PORT)
repl:
	mpremote connect $(PORT) repl
clean:
	rm -rf .pytest_cache **/__pycache__
