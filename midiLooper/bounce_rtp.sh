#!/bin/sh

MPC_IP=$(arp -i eth0 | awk 'NR>1 {print $1}')
for CANDIDATE in $MPC_IP; do
    if nc -z -w 1 "$CANDIDATE" 22 2>/dev/null; then
        MPC_IP="$CANDIDATE"
        break
    fi
done

ssh -i /home/pi/.ssh/n8n root@"$MPC_IP" 'systemctl restart rtpmidi'

sudo service rtpmidid restart

