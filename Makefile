
TARGET=decode-backup encode2backup

all : $(TARGET)

decode-backup: decode-backup.c
	@gcc -g -o $@ $<
	@ls -alF $@

encode2backup : encode2backup.c
	@gcc -g -o $@ $<
	@ls -alF $@

clean:
	@rm -f $(TARGET)
