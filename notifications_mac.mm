/*
    ClipGrab³
    Copyright (C) Philipp Schmieder
    http://clipgrab.de
    feedback [at] clipgrab [dot] de

    This file is part of ClipGrab.
    ClipGrab is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    ClipGrab is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with ClipGrab.  If not, see <http://www.gnu.org/licenses/>.
*/



#include "notifications_mac.h"
#include "Foundation/Foundation.h"


void NSUserNotifications::showMessage(QString title, QString message) {

    Class NSUserNotificationClass =  NSClassFromString(@"NSUserNotification");
    if (NSUserNotificationClass)
    {

        id NSUserNotification = [[[NSUserNotificationClass alloc] init] autorelease];
        NSString *nTitle = [[NSString alloc] initWithUTF8String:title.toUtf8().data()];
        NSString *nMessage = [[NSString alloc] initWithUTF8String:message.toUtf8().data()];
        [NSUserNotification setTitle: nTitle];
        [NSUserNotification setInformativeText: nMessage];

        [[NSClassFromString(@"NSUserNotificationCenter") defaultUserNotificationCenter] deliverNotification:NSUserNotification];
    }
}
