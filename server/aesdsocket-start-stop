#!/bin/sh

### BEGIN INIT INFO
# Provides:          aesdsocket
# Required-Start:    $remote_fs $syslog
# Required-Stop:     $remote_fs $syslog
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: AESD socket daemon
# Description:       Start the AESD socket server daemon.
### END INIT INFO

DAEMON=/usr/bin/aesdsocket
DAEMON_OPTS="-d"
NAME=aesdsocket
DESC="AESD socket server"
PIDFILE=/var/run/$NAME.pid

. /lib/init/vars.sh
. /lib/lsb/init-functions

do_start() {
    start-stop-daemon --start --background --quiet --make-pidfile --pidfile $PIDFILE --exec $DAEMON -- $DAEMON_OPTS || return 1
    log_end_msg $?
}

do_stop() {
    start-stop-daemon --stop --quiet --pidfile $PIDFILE --retry=TERM/10/KILL/5 || return 1
    rm -f $PIDFILE
    log_end_msg $?
}

case "$1" in
    start)
        log_daemon_msg "Starting $DESC" $NAME
        do_start
        ;;
    stop)
        log_daemon_msg "Stopping $DESC" $NAME
        do_stop
        ;;
    restart|force-reload)
        log_daemon_msg "Restarting $DESC" $NAME
        do_stop
        sleep 1
        do_start
        ;;
    status)
        status_of_proc "$DAEMON" "$NAME" && exit 0 || exit $?
        ;;
    *)
        echo "Usage: $0 {start|stop|status|restart|force-reload}" >&2
        exit 3
        ;;
esac

exit 0

