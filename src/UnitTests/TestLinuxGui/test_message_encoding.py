#!/usr/bin/env python3
from support import context, passed
ctx = context('message-encoding')
ctx.runtime()
ctx.gui(ctx.build_gui('MessageEncodingRegression.cpp'), 'messages')
passed('UTF-8 messages, dialogs, percent signs and empty text')
