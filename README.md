# edid_fixer

A Linux kernel module that replaces a specific display's EDID

## Why?
I have an issue where my AOC 27G1G4 monitor which I hooked up to my Steam Deck via an unofficial USB-C hub doesn't show a refresh rate of >60Hz when I plug in my second monitor.

## So why not just use edid_override?
I did that, but it was annoying to do it manually every time and then re-plug the monitor.

Also, I couldn't automate it properly because my DP-2 connector may have another 60Hz monitor plugged in when I'm not using my setup at home.
