.PHONY: all clean distclean server client

all: server client
	@echo "✓ Compilation terminée: serveur et client"

server:
	$(MAKE) -C server

client:
	$(MAKE) -C client

clean:
	$(MAKE) -C server clean
	$(MAKE) -C client clean

distclean:
	$(MAKE) -C server distclean
	$(MAKE) -C client distclean
