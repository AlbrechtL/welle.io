/*
 * MOT header extension bounds regression tests.
 * This file was generated with the help of an AI assistant
 * (OpenAi Codex 6 Astra).
 *
 * welle.io is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * welle.io is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with welle.io; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 */

#include <QtTest>
#include <cstring>
#include "mot_manager.h"

namespace {
bool parse(const QByteArray& bytes, MOT_FILE& file, std::string& name)
{
    return ParseMOTHeaderExtension(
        reinterpret_cast<const uint8_t*>(bytes.constData()), size_t(bytes.size()), file, name);
}
}

class TestMotHeader : public QObject {
    Q_OBJECT
private slots:
    void emptyAndDuplicateSegments() {
        const uint8_t empty = 0;
        MOTEntity entity;
        entity.AddSeg(0, false, &empty, 0);
        const uint8_t payload[] = {1, 2, 3};
        entity.AddSeg(1, true, payload, sizeof(payload));
        entity.AddSeg(1, true, payload, sizeof(payload));
        QVERIFY(entity.IsFinished());
        QCOMPARE(entity.GetSize(), size_t(3));
        QCOMPARE(entity.GetData(), std::vector<uint8_t>({1,2,3}));
        entity.Reset();
        entity.AddSeg(0, true, &empty, 0);
        QVERIFY(entity.IsFinished());
        QVERIFY(entity.GetData().empty());
    }

    void ucs2ContentName_data() {
        QTest::addColumn<QByteArray>("payload");
        QTest::addColumn<bool>("accepted");
        QTest::newRow("incomplete-code-unit") << QByteArray(1, char(0x41)) << false;
        for (const auto& row : std::vector<std::pair<const char*, char16_t>>{
                 {"unaligned-ascii", u'A'}, {"unpaired-low-surrogate", 0xdc00},
                 {"unpaired-high-surrogate", 0xd800}}) {
            QByteArray payload(sizeof(char16_t), char(0));
            std::memcpy(payload.data(), &row.second, sizeof(char16_t));
            QTest::newRow(row.first) << payload << (row.second == u'A');
        }
    }
    void ucs2ContentName() {
        QFETCH(QByteArray, payload);
        QFETCH(bool, accepted);
        QByteArray bytes;
        bytes.append(char(0xcc));
        bytes.append(char(payload.size() + 1));
        bytes.append(char(0x60));
        bytes.append(payload);
        MOT_FILE file;
        std::string name;
        QCOMPARE(parse(bytes, file, name), accepted);
        if (accepted) QCOMPARE(name, std::string("A"));
    }

    void parameterBounds_data() {
        QTest::addColumn<QByteArray>("bytes");
        QTest::addColumn<bool>("accepted");
        const auto row = [](const char* label, const char* hex, bool ok) {
            QTest::newRow(label) << QByteArray::fromHex(hex) << ok;
        };
        row("empty-extension", "", true);
        row("unknown-zero-length", "3f", true);
        row("unknown-one-byte-exact", "7faa", true);
        row("unknown-one-byte-missing", "7f", false);
        row("unknown-four-byte-exact", "bf01020304", true);
        row("unknown-four-byte-truncated", "bf010203", false);
        row("variable-length-byte-missing", "ff", false);
        row("extended-length-byte-missing", "ff80", false);
        row("variable-zero-length", "ff00", true);
        row("extended-zero-length", "ff8000", true);
        row("variable-length-exact", "ff03aabbcc", true);
        row("variable-length-truncated", "ff03aabb", false);
        row("extended-length-exact", "ff8002aabb", true);
        row("extended-length-truncated", "ff8003aabb", false);
        row("maximum-length-truncated", "ffffff", false);
        row("expiry-without-payload", "04", false);
        row("expiry-variable-empty", "c400", false);
        row("expiry-before-next-parameter", "0463ff", false);
        row("trigger-too-short", "4500", false);
        row("content-name-without-charset", "0c", false);
        row("content-name-truncated", "cc03f061", false);
    }

    void parameterBounds() {
        QFETCH(QByteArray, bytes);
        QFETCH(bool, accepted);
        MOT_FILE file;
        file.expire_time = 42;
        std::string name;
        QCOMPARE(parse(bytes, file, name), accepted);
        // Rejected empty expiry must not read the following parameter's ID.
        QCOMPARE(file.expire_time, uint32_t(42));
    }

    void emptyStrings_data() {
        QTest::addColumn<QByteArray>("bytes");
        QTest::newRow("mime") << QByteArray::fromHex("10");
        QTest::newRow("category-title") << QByteArray::fromHex("26");
        QTest::newRow("click-through") << QByteArray::fromHex("27");
        // Following CAInfo (0x23) and TriggerTime (0x85, four zero bytes)
        // provide a nonzero sentinel and an in-bounds terminator for all three
        // charsets. The old zero-length C-string conversion must consume the
        // next field, rather than accidentally finding zero beyond the buffer.
        QTest::newRow("name-ebu") << QByteArray::fromHex("4c00238500000000");
        QTest::newRow("name-ucs2") << QByteArray::fromHex("4c60238500000000");
        QTest::newRow("name-utf8") << QByteArray::fromHex("4cf0238500000000");
    }

    void emptyStrings() {
        QFETCH(QByteArray, bytes);
        MOT_FILE file;
        file.content_name = file.mime_type = file.category_title = file.click_through_url = "old";
        std::string name = "old";
        QVERIFY(parse(bytes, file, name));
        switch (uint8_t(bytes[0]) & 0x3f) {
        case 0x0c:
            QVERIFY(file.content_name.empty());
            QVERIFY(name.empty());
            QVERIFY(file.ca_info);
            QVERIFY(file.trigger_time_now);
            break;
        case 0x10: QVERIFY(file.mime_type.empty()); break;
        case 0x26: QVERIFY(file.category_title.empty()); break;
        case 0x27: QVERIFY(file.click_through_url.empty()); break;
        }
    }

    void validFieldsAndFollowingParameter() {
        MOT_FILE file;
        std::string name;
        // Expiry, trigger, UTF-8 name, MIME, category/slide, title, click-through,
        // followed by a zero-length CAInfo parameter. No field may swallow it.
        const auto bytes = QByteArray::fromHex(
            "442a8500000000cc04f0616263d003612f62e5020709e60178e7017923");
        QVERIFY(parse(bytes, file, name));
        QCOMPARE(file.expire_time, uint32_t(42));
        QVERIFY(file.trigger_time_now);
        QCOMPARE(file.content_name, std::string("abc"));
        QCOMPARE(name, std::string("abc"));
        QCOMPARE(file.mime_type, std::string("a/b"));
        QCOMPARE(file.category, uint8_t(7));
        QCOMPARE(file.slide_id, uint8_t(9));
        QCOMPARE(file.category_title, std::string("x"));
        QCOMPARE(file.click_through_url, std::string("y"));
        QVERIFY(file.ca_info);
    }
};

QTEST_GUILESS_MAIN(TestMotHeader)
#include "tst_mot_header.moc"
