// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef BASIC_H_
#define BASIC_H_
#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>

namespace Methods
{
	struct Basic
	{
		QString type;
	};

	inline void fromJson(const QByteArray &array, Basic &basic)
	{
		QJsonDocument doc = QJsonDocument::fromJson(array);
		if (!doc.isObject()) {
			qWarning() << "fromJson basic failed";
			return;
		}

		QJsonObject obj = doc.object();
		if (!obj.contains("type")) {
			qWarning() << "type not exist in basic array";
			return;
		}

		basic.type = obj.value("type").toString();
	}

} // namespace Methods

#endif // BASIC_H_
