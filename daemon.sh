#! /bin/sh
# /etc/init.d/geigercounter

### BEGIN INIT INFO
# Provides:          geigercounter
# Required-Start:    $remote_fs $syslog
# Required-Stop:     $remote_fs $syslog
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: Start daemon at boot time
# Description:       Enable service provided by daemon.
### END INIT INFO


DAEMON=<place path to geiger.elf executable here>
NAME=geigercounter

test -x $DAEMON || exit 0

case "$1" in
    start)
    echo -n "Starting dynamic address update: "
    start-stop-daemon --start --exec $DAEMON
    echo "geiger counter."
    ;;
    stop)
    echo -n "Shutting down dynamic address update:"
    start-stop-daemon --stop --oknodo --retry 30 --exec $DAEMON
    echo "geiger counter."
    ;;

    restart)
    echo -n "Restarting dynamic address update: "
    start-stop-daemon --stop --oknodo --retry 30 --exec $DAEMON
    start-stop-daemon --start --exec $DAEMON
    echo "geiger counter."
    ;;

    *)
    echo "Usage: $0 {start|stop|restart}"
    exit 1
esac
exit 0