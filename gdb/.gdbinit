# If there is an error concerning security, please add:
#   set auto-load safe-path /
# to your ~/.gdbinit

# Turns on or off display of reports on all packets sent back and forth across
# the serial line to the remote machine. The info is printed on the GDB standard
# output stream. The default is off.
# set debug remote off

# Turns on or off display of GDB target debugging info. This info includes what
# is going on at the target level of GDB, as it happens. The default is off (0).
# set debug target off

target remote :1234
