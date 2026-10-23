BINARY_DIRS := c1 c1xx c2 cl cvppack cvtres link rcdll rc

.PHONY: all inspect $(BINARY_DIRS) native original fixtures
all: inspect
inspect: $(BINARY_DIRS)

$(BINARY_DIRS):
	$(MAKE) -C $@ inspect

native original fixtures:
	@echo 'VC 5.10 groundwork only; reconstruction has not been released.' >&2
	@exit 2
