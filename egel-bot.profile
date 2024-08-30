# Firejail profile for egel-bot

# Name and description
name egel-bot
description A custom profile for egel-bot

# Private /tmp and /var/tmp directories
private-tmp

# Private dev and run directories
private-dev
private-run

# Private etc directory (prevents the app from seeing system configurations)
private-etc

# Whitelist specific directories that your application needs access to
whitelist ${HOME}/Programming/egel-bot/build
#whitelist ${HOME}/.config/egel-bot  # Config directory if needed

# No network access (uncomment if you want to restrict network access)
# nonewprivs
# net none

# Allow the app to use the network, but restrict it to localhost only
# Uncomment if you want the app to only access local services
# netfilter
# netfilter 127.0.0.1

# Optional: Enable read-only mode for the home directory
# read-only ${HOME}

# Enable private home directory (commented out since we're whitelisting specific directories)
# private-home

# Memory restrictions (optional)
rlimit-as 512M
rlimit-fsize 100M

# Capabilities restrictions (reduces attack surface)
caps.drop all

# Disable access to specific features
seccomp
noroot

# Disable unwanted program execution (whitelist specific programs if necessary)
shell none
exec ${HOME}/Programming/egel-bot/egel-bot

# End of profile
