/*
 * Copyright (C) 2022 ~ 2023 Deepin Technology Co., Ltd.
 *
 * Author:     weizhixiang <weizhixiang@uniontech.com>
 *
 * Maintainer: weizhixiang <weizhixiang@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "xcbutils.h"

#include <iostream>
#include <cstring>

XCBUtils::XCBUtils()
{
    connect = xcb_connect(nullptr, &screenNum); // nullptr表示默认使用环境变量$DISPLAY获取屏幕
    if (xcb_connection_has_error(connect)) {
        std::cout << "XCBUtils: init xcb_connect error" << std::endl;
        return;
    }

    if (!xcb_ewmh_init_atoms_replies(&ewmh,
                                     xcb_ewmh_init_atoms(connect, &ewmh),   // 初始化Atom
                                     nullptr))
        std::cout << "XCBUtils: init ewmh  error" << std::endl;
}

XCBUtils::~XCBUtils()
{
    if (connect) {
        xcb_disconnect(connect);    // 关闭连接并释放
        connect = nullptr;
    }
}

XWindow XCBUtils::allocId()
{
    return xcb_generate_id(connect);
}

void XCBUtils::killClientChecked(XWindow xid)
{
    xcb_kill_client_checked(connect, xid);
}

xcb_get_property_reply_t *XCBUtils::getPropertyValueReply(XWindow xid, XCBAtom property, XCBAtom type)
{
    xcb_get_property_cookie_t cookie = xcb_get_property(connect,
                                                        0,
                                                        xid,
                                                        property,
                                                        type,
                                                        0,
                                                        MAXLEN);
    return xcb_get_property_reply(connect, cookie, nullptr);
}

void *XCBUtils::getPropertyValue(XWindow xid, XCBAtom property, XCBAtom type)
{
    void *value = nullptr;
    xcb_get_property_reply_t *reply = getPropertyValueReply(xid, property, type);
    if (reply) {
        if (xcb_get_property_value_length(reply) > 0) {
            value = xcb_get_property_value(reply);
        }
        free(reply);
    }
    return value;
}

std::string XCBUtils::getUTF8PropertyStr(XWindow xid, XCBAtom property)
{
    std::string ret;
    xcb_get_property_reply_t *reply = getPropertyValueReply(xid, property, ewmh.UTF8_STRING);
    if (reply) {
        ret = getUTF8StrFromReply(reply);

        free(reply);
    }
    return ret;
}

XCBAtom XCBUtils::getAtom(const char *name)
{
    XCBAtom ret = atomCache.getVal(name);
    if (ret == ATOMNONE) {
        xcb_intern_atom_cookie_t cookie = xcb_intern_atom(connect, false, strlen(name), name);
        xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply (connect,
                                                                cookie,
                                                                nullptr);
        if (reply) {
            atomCache.store(name, reply->atom);
            ret = reply->atom;

            free(reply);
        }
    }

    return ret;
}

std::string XCBUtils::getAtomName(XCBAtom atom)
{
    std::string ret = atomCache.getName(atom);
    if (ret.empty()) {
        xcb_get_atom_name_cookie_t cookie = xcb_get_atom_name(connect, atom);
        xcb_get_atom_name_reply_t *reply = xcb_get_atom_name_reply(connect,
                                                                   cookie,
                                                                   nullptr);
        if (reply) {
            char *name = xcb_get_atom_name_name(reply);
            if (name) {
                atomCache.store(name, atom);
                ret = name;
            }

            free(reply);
        }
    }

    return ret;
}

Geometry XCBUtils::getWindowGeometry(XWindow xid)
{
    Geometry ret;
    xcb_get_geometry_cookie_t cookie = xcb_get_geometry(connect, xid);
    xcb_get_geometry_reply_t *reply = xcb_get_geometry_reply(connect, cookie, nullptr);
    if (reply) {
        ret.x = reply->x;
        ret.y = reply->y;
        ret.width = reply->width;
        ret.height = reply->height;

        free(reply);
    } else {
        std::cout << xid << " getWindowGeometry err" << std::endl;
    }

    return ret;
}

XWindow XCBUtils::getActiveWindow()
{
   XWindow ret;
   xcb_get_property_cookie_t cookie = xcb_ewmh_get_active_window(&ewmh, screenNum);
   if (!xcb_ewmh_get_active_window_reply(&ewmh, cookie, &ret, nullptr))
       std::cout << "getActiveWindow error" << std::endl;

   return ret;
}

void XCBUtils::setActiveWindow(XWindow xid)
{
    xcb_ewmh_set_active_window(&ewmh, screenNum, xid);
}

std::list<XWindow> XCBUtils::getClientList()
{
    std::list<XWindow> ret;
    xcb_get_property_cookie_t cookie = xcb_ewmh_get_client_list(&ewmh, screenNum);
    xcb_ewmh_get_windows_reply_t reply;
    if (!xcb_ewmh_get_client_list_reply(&ewmh, cookie, &reply, nullptr))
        std::cout << "getClientList error" << std::endl;

    for (uint32_t i = 0; i < reply.windows_len; i++)
        ret.push_back(reply.windows[i]);

    return ret;
}

std::list<XWindow> XCBUtils::getClientListStacking()
{
    std::list<XWindow> ret;
    xcb_get_property_cookie_t cookie = xcb_ewmh_get_client_list_stacking(&ewmh, screenNum);
    xcb_ewmh_get_windows_reply_t reply;
    if (!xcb_ewmh_get_client_list_stacking_reply(&ewmh, cookie, &reply, nullptr))
        std::cout << "getClientListStacking error" << std::endl;

    for (uint32_t i = 0; i < reply.windows_len; i++)
        ret.push_back(reply.windows[i]);

    return ret;
}

std::vector<XCBAtom> XCBUtils::getWMState(XWindow xid)
{
    std::vector<XCBAtom> ret;
    xcb_get_property_cookie_t cookie = xcb_ewmh_get_wm_state(&ewmh, xid);
    xcb_ewmh_get_atoms_reply_t reply; // a list of Atom
    if (xcb_ewmh_get_wm_state_reply(&ewmh, cookie, &reply, nullptr)) {
        for (uint32_t i = 0; i < reply.atoms_len; i++) {
            ret.push_back(reply.atoms[i]);
        }
    } else {
        std::cout << xid << " getWMState error" << std::endl;
    }

    return ret;
}

std::vector<XCBAtom> XCBUtils::getWMWindoType(XWindow xid)
{
    std::vector<XCBAtom> ret;
    xcb_get_property_cookie_t cookie = xcb_ewmh_get_wm_window_type(&ewmh, xid);
    xcb_ewmh_get_atoms_reply_t reply; // a list of Atom
    if (!xcb_ewmh_get_wm_window_type_reply(&ewmh, cookie, &reply, nullptr))
        std::cout << xid << " getWMWindoType error" << std::endl;

    return ret;
}

std::vector<XCBAtom> XCBUtils::getWMAllowedActions(XWindow xid)
{
    std::vector<XCBAtom> ret;
    xcb_get_property_cookie_t cookie = xcb_ewmh_get_wm_allowed_actions(&ewmh, xid);
    xcb_ewmh_get_atoms_reply_t reply;   // a list of Atoms
    if (!xcb_ewmh_get_wm_allowed_actions_reply(&ewmh, cookie, &reply, nullptr))
        std::cout << xid << " getWMAllowedActions error" << std::endl;

    for (uint32_t i = 0; i < reply.atoms_len; i++) {
        ret.push_back(reply.atoms[i]);
    }

    return ret;
}

void XCBUtils::setWMAllowedActions(XWindow xid, std::vector<XCBAtom> actions)
{
    XCBAtom list[MAXALLOWEDACTIONLEN] {0};
    for (size_t i = 0; i < actions.size(); i++)
        list[i] = actions[i];

    xcb_ewmh_set_wm_allowed_actions(&ewmh, xid, actions.size(), list);
}

std::string XCBUtils::getWMName(XWindow xid)
{
    std::string ret;
    xcb_get_property_cookie_t cookie = xcb_ewmh_get_wm_name(&ewmh, xid);
    xcb_ewmh_get_utf8_strings_reply_t reply1;
    if (!xcb_ewmh_get_wm_name_reply(&ewmh, cookie, &reply1, nullptr))
        std::cout << xid << " getWMName error" << std::endl;

    ret.assign(reply1.strings);

    return ret;
}

uint32_t XCBUtils::getWMPid(XWindow xid)
{
    uint32_t ret = 0;
    xcb_get_property_cookie_t cookie = xcb_ewmh_get_wm_pid(&ewmh, xid);
    if (!xcb_ewmh_get_wm_pid_reply(&ewmh, cookie, &ret, nullptr))
        std::cout << xid << " getWMPid error" << std::endl;

    return ret;
}

std::string XCBUtils::getWMIconName(XWindow xid)
{
    std::string ret;
    xcb_get_property_cookie_t cookie = xcb_ewmh_get_wm_icon_name(&ewmh, xid);
    xcb_ewmh_get_utf8_strings_reply_t reply;
    if (!xcb_ewmh_get_wm_icon_name_reply(&ewmh, cookie, &reply, nullptr))
        std::cout << xid << " getWMIconName error" << std::endl;

    ret.assign(reply.strings);

    return ret;
}

std::vector<WMIcon> XCBUtils::getWMIcon(XWindow xid)
{
    std::vector<WMIcon> ret;
    /*
    xcb_get_property_cookie_t cookie = xcb_ewmh_get_wm_icon(&ewmh, xid);
    xcb_ewmh_get_wm_icon_reply_t reply;
    if (xcb_ewmh_get_wm_icon_reply(&ewmh, cookie, &reply, nullptr)) {
        xcb_ewmh_wm_icon_iterator_t iter = xcb_ewmh_get_wm_icon_iterator(&reply);
        auto fcn = [](xcb_ewmh_wm_icon_iterator_t it) {
            std::vector<BYTE> data;
            uint32_t *dat = it.data;
            int area = it.width * it.height;
            for (int i = 0; i < (2 + area) * 4; i++, dat++) { // TODO check data accuracy
                data.push_back(*dat);
            }
            return data;
        };

        ret.push_back({iter.width, iter.height, fcn(iter)});

        while (iter.rem >= 1) {
            xcb_ewmh_get_wm_icon_next(&iter);
            ret.push_back({iter.width, iter.height, fcn(iter)});
        }

        xcb_ewmh_get_wm_icon_reply_wipe(&reply); // clear
    }
*/
    return ret;
}

XWindow XCBUtils::getWMClientLeader(XWindow xid)
{
    XWindow ret;
    XCBAtom atom = getAtom("WM_CLIENT_LEADER");
    void *value = getPropertyValue(xid, atom, XCB_ATOM_INTEGER);
    std::cout << "getWMClientLeader:" << (char*)value << std::endl;

    return ret;
}

void XCBUtils::requestCloseWindow(XWindow xid, uint32_t timestamp)
{
    xcb_ewmh_request_close_window(&ewmh, screenNum, xid, timestamp, XCB_EWMH_CLIENT_SOURCE_TYPE_OTHER);
}

uint32_t XCBUtils::getWMDesktop(XWindow xid)
{
    uint32_t ret;
    xcb_get_property_cookie_t cookie = xcb_ewmh_get_wm_desktop(&ewmh, xid);
    if (!xcb_ewmh_get_wm_desktop_reply(&ewmh, cookie, &ret, nullptr))
        std::cout << xid << " getWMDesktop error" << std::endl;

    return ret;
}

void XCBUtils::setWMDesktop(XWindow xid, uint32_t desktop)
{
    xcb_ewmh_set_wm_desktop(&ewmh, xid, desktop);
}

void XCBUtils::setCurrentWMDesktop(uint32_t desktop)
{
    xcb_ewmh_set_current_desktop(&ewmh, screenNum, desktop);
}

uint32_t XCBUtils::getCurrentWMDesktop()
{
    uint32_t ret;
    xcb_get_property_cookie_t cookie = xcb_ewmh_get_current_desktop(&ewmh, screenNum);
    if (!xcb_ewmh_get_current_desktop_reply(&ewmh, cookie, &ret, nullptr))
        std::cout << "getCurrentWMDesktop error" << std::endl;

    return ret;
}

bool XCBUtils::isGoodWindow(XWindow xid)
{
    bool ret = false;
    xcb_get_geometry_cookie_t cookie = xcb_get_geometry(connect, xid);
    xcb_generic_error_t **errStore = nullptr;
    xcb_get_geometry_reply_t *reply = xcb_get_geometry_reply(connect, cookie, errStore);
    if (reply) {
        if (!errStore)      // 正常获取窗口geometry则判定为good
            ret = true;

        free(reply);
    }
    return ret;
}

// TODO XCB下无_MOTIF_WM_HINTS属性
MotifWMHints XCBUtils::getWindowMotifWMHints(XWindow xid)
{
    MotifWMHints ret;

    return ret;
}

bool XCBUtils::hasXEmbedInfo(XWindow xid)
{
    //XCBAtom atom = getAtom("_XEMBED_INFO");

    return false;
}

XWindow XCBUtils::getWMTransientFor(XWindow xid)
{
    XWindow ret;
    xcb_get_property_cookie_t cookie = xcb_icccm_get_wm_transient_for(connect, xid);
    if (!xcb_icccm_get_wm_transient_for_reply(connect, cookie, &ret, nullptr))
        std::cout << xid << " getWMTransientFor error" << std::endl;

    return ret;
}

uint32_t XCBUtils::getWMUserTime(XWindow xid)
{
    uint32_t ret;
    xcb_get_property_cookie_t cookie = xcb_ewmh_get_wm_user_time(&ewmh, xid);
    if (!xcb_ewmh_get_wm_user_time_reply(&ewmh, cookie, &ret, nullptr))
        std::cout << xid << " getWMUserTime error" << std::endl;

    return ret;
}

int XCBUtils::getWMUserTimeWindow(XWindow xid)
{
    XCBAtom ret;
    xcb_get_property_cookie_t cookie = xcb_ewmh_get_wm_user_time_window(&ewmh, xid);
    if (!xcb_ewmh_get_wm_user_time_window_reply(&ewmh, cookie, &ret, NULL))
        std::cout << xid << " getWMUserTimeWindow error" << std::endl;

    return ret;
}

WMClass XCBUtils::getWMClass(XWindow xid)
{
    WMClass ret;
    xcb_get_property_cookie_t cookie = xcb_icccm_get_wm_class(connect, xid);
    xcb_icccm_get_wm_class_reply_t reply;
    if (!xcb_icccm_get_wm_class_reply(connect, cookie, &reply, nullptr)) {
        if (reply.class_name)
            ret.className.assign(reply.class_name);

        if (reply.instance_name)
            ret.instanceName.assign(reply.instance_name);

        //xcb_icccm_get_wm_class_reply_wipe(&reply);
    } else {
        std::cout << xid << " getWMClass error" << std::endl;
    }

    return ret;
}

// TODO
void XCBUtils::minimizeWindow(XWindow xid)
{
    xcb_get_property_cookie_t cookie = xcb_icccm_get_wm_hints(connect, xid);
    xcb_icccm_wm_hints_t *hints = new xcb_icccm_wm_hints_t; // 分配堆空间
    xcb_icccm_get_wm_hints_reply(connect, cookie, hints, nullptr);
    xcb_icccm_wm_hints_set_iconic(hints);
    xcb_icccm_set_wm_hints(connect, xid, hints);
    free(hints);
}

void XCBUtils::maxmizeWindow(XWindow xid)
{
    xcb_ewmh_request_change_wm_state(&ewmh
                                     , screenNum
                                     , xid
                                     , XCB_EWMH_WM_STATE_ADD
                                     , getAtom("_NET_WM_STATE_MAXIMIZED_VERT")
                                     , getAtom("_NET_WM_STATE_MAXIMIZED_HORZ")
                                     , XCB_EWMH_CLIENT_SOURCE_TYPE_OTHER);
}

// TODO
std::vector<std::string> XCBUtils::getWMCommand(XWindow xid)
{
    std::vector<std::string> ret;
    xcb_get_property_reply_t *reply = getPropertyValueReply(xid, XCB_ATOM_WM_COMMAND, ewmh.UTF8_STRING);
    if (reply) {
        ret = getUTF8StrsFromReply(reply);
        free(reply);
    }

    return ret;
}

std::string XCBUtils::getUTF8StrFromReply(xcb_get_property_reply_t *reply)
{
    std::string ret;
    if (!reply || reply->format != 8)
        return ret;

    char data[12] = {0};
    for (uint32_t i=0; i < reply->value_len; i++) {
        data[i] = char(reply->pad0[i]);
    }
    ret.assign(data);
    return ret;
}

std::vector<std::string> XCBUtils::getUTF8StrsFromReply(xcb_get_property_reply_t *reply)
{
    std::vector<std::string> ret;
    if (!reply)
        return ret;

    if (reply->format != 8)
        return ret;


    // 字符串拆分
    uint32_t start = 0;
    for (uint32_t i=0; i < reply->value_len; i++) {
        if (reply->pad0[i] == 0) {
            char data[12] = {0};
            int count = 0;
            for (uint32_t j=start; j < i; j++)
                data[count++] = char(reply->pad0[j]);

            data[count] = 0;
            ret.push_back(data);
        }
    }

    return ret;
}

XWindow XCBUtils::getRootWindow()
{
    XWindow rootWindow = 0;
    /* Get the first screen */
    xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(connect)).data;
    if (screen)
        rootWindow = screen->root;

    std::cout << "getRootWinodw: " << rootWindow << std::endl;
    return rootWindow;
}

void XCBUtils::registerEvents(XWindow xid, uint32_t eventMask)
{
    uint32_t value[1] = {eventMask};
    xcb_void_cookie_t cookie = xcb_change_window_attributes_checked(connect,
                                                                    xid,
                                                                    XCB_CW_EVENT_MASK,
                                                                    &value);
    xcb_flush(connect);

    xcb_generic_error_t *error = xcb_request_check(connect, cookie);
    if (error != nullptr) {
        std::cout << "window " << xid << "registerEvents error" << std::endl;
    }
}


AtomCache::AtomCache()
{
}

XCBAtom AtomCache::getVal(std::string name)
{
    XCBAtom atom = ATOMNONE;
    auto search = atoms.find(name);
    if (search != atoms.end())
        atom = search->second;

    return atom;
}

std::string AtomCache::getName(XCBAtom atom)
{
    std::string ret;
    auto search = atomNames.find(atom);
    if (search != atomNames.end())
        ret = search->second;

    return ret;
}

void AtomCache::store(std::string name, XCBAtom value)
{
    atoms[name] = value;
    atomNames[value] = name;
}
