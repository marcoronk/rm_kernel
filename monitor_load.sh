#!/bin/sh
module=sys_module
folder=/opt/monitor


if [ -e ${module}.ko ]; then
    echo "Loading ${module}.ko"
    insmod $folder/$module.ko $* || exit 1
else
    echo "Local file ${module}.ko not found, attempting to modprobe"
    modprobe ${module} || exit 1
fi

