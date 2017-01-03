# Create the Makefile.lcl, which should define
# MOD_PATH, the folder where the apache module should reside
# For example, loot at Makefile.lcl.example

TARGET = mod_ahtse_lua

include Makefile.lcl

default:	$(TARGET).cpp $(TARGET).h
	apxs -c $<

# Only do this when httpd is not running
install :       ./.libs/$(TARGET).so
	sudo    cp ./.libs/$(TARGET).so    $(MOD_PATH)
