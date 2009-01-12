"""
beep.py - prints a beep (\\a) when receiving a privmsg or highlight

Author: Bogdano Arendartchuk <bhdn@ukr.net>
"""
import os
import weechat

def unload():
    return weechat.PLUGIN_RC_OK

def add_message(server, args):
    os.system("echo -en '\\a'")
    return weechat.PLUGIN_RC_OK

weechat.register("beep", "0.01", "unload", "Beep for weechat in Python")
weechat.add_message_handler("weechat_highlight", "add_message")
weechat.add_message_handler("privmsg", "add_message")
