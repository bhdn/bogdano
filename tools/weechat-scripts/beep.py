"""
beep.py - prints a beep (\\a) when receiving a privmsg or highlight

To install this script you need the "weechat-python" package (or the
equivalent for your distro) installed. Then copy this file to
~/.weechat/python/autoload/

Author: Bogdano Arendartchuk <bhdn@ukr.net>
"""
import os
import weechat

def unload():
    return weechat.PLUGIN_RC_OK

def highlight(server, args):
    os.system("echo -en '\\a'")
    return weechat.PLUGIN_RC_OK

def pv(server, args):
    if "PRIVMSG #" not in args:
        return highlight(server, args)
    return weechat.PLUGIN_RC_OK

weechat.register("beep", "0.01", "unload", "Beep for weechat in Python")
weechat.add_message_handler("weechat_highlight", "highlight")
weechat.add_message_handler("privmsg", "pv")
