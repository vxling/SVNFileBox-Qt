#include "newfileservice.h"
#include <QFile>
#include <QDir>
#include <QByteArray>
#include <cstdio>
#include <cstring>

// ─── CRC32 (PNG / zlib polynomial 0xEDB88320) ──────────────────────────────────
static quint32 crc32_table[256];
static bool crc32_init = false;

static void ensureCrc32()
{
    if (crc32_init) return;
    for (quint32 i = 0; i < 256; ++i) {
        quint32 c = i;
        for (int j = 0; j < 8; ++j)
            c = (c & 1) ? (0xEDB88320U ^ (c >> 1)) : (c >> 1);
        crc32_table[i] = c;
    }
    crc32_init = true;
}

static quint32 crc32(const QByteArray &data)
{
    ensureCrc32();
    quint32 crc = 0xFFFFFFFF;
    for (unsigned char b : data)
        crc = crc32_table[(crc ^ b) & 0xFF] ^ (crc >> 8);
    return crc ^ 0xFFFFFFFF;
}

static quint32 crc32(const void *ptr, size_t len)
{
    return crc32(QByteArray(static_cast<const char*>(ptr), len));
}

// ─── PNG helpers ──────────────────────────────────────────────────────────────
static QByteArray pngChunk(const char *type, const QByteArray &data)
{
    QByteArray chunk;
    // length (big-endian 4 bytes)
    quint32 len = data.size();
    chunk.append(static_cast<char>(len >> 24));
    chunk.append(static_cast<char>(len >> 16));
    chunk.append(static_cast<char>(len >> 8));
    chunk.append(static_cast<char>(len));
    // type + data
    chunk.append(type, 4);
    chunk.append(data);
    // CRC of type + data
    QByteArray typeAndData = QByteArray(type, 4) + data;
    quint32 crc = crc32(typeAndData.constData(), typeAndData.size());
    chunk.append(static_cast<char>(crc >> 24));
    chunk.append(static_cast<char>(crc >> 16));
    chunk.append(static_cast<char>(crc >> 8));
    chunk.append(static_cast<char>(crc));
    return chunk;
}

// ─── ZIP helpers (stored / deflate) ──────────────────────────────────────────
static void zipWriteHeader(QByteArray &out, const QString &name, const QByteArray &content, quint32 crcVal)
{
    QByteArray nameBytes = name.toUtf8();
    quint16 nameLen = static_cast<quint16>(nameBytes.size());
    quint16 contentLen = static_cast<quint16>(content.size());
    quint32 compSize = content.size();
    quint32 uncompSize = content.size();

    // Local file header (30 + nameLen bytes)
    out.append("\x50\x4b\x03\x04");        // signature
    out.append("\x14\x00");                // version needed (2.0)
    out.append("\x00\x00");                // flags, compression (stored)
    out.append("\x00\x00");                // compression method 0 = stored
    out.append("\x00\x00");                // mod time
    out.append("\x00\x00");                // mod date
    out.append(static_cast<char>(crcVal >> 24));
    out.append(static_cast<char>(crcVal >> 16));
    out.append(static_cast<char>(crcVal >> 8));
    out.append(static_cast<char>(crcVal));
    out.append(static_cast<char>(compSize >> 24));
    out.append(static_cast<char>(compSize >> 16));
    out.append(static_cast<char>(compSize >> 8));
    out.append(static_cast<char>(compSize));
    out.append(static_cast<char>(uncompSize >> 24));
    out.append(static_cast<char>(uncompSize >> 16));
    out.append(static_cast<char>(uncompSize >> 8));
    out.append(static_cast<char>(uncompSize));
    out.append(static_cast<char>(nameLen >> 8));
    out.append(static_cast<char>(nameLen));
    out.append("\x00\x00");                // extra field length
    out.append(nameBytes);

    // File data
    out.append(content);
}

struct ZipEntry { QString name; QByteArray content; };
static QByteArray buildZip(const QList<ZipEntry> &entries)
{
    QByteArray out;
    QByteArray central;
    quint32 offset = 0;

    for (const ZipEntry &e : entries) {
        QByteArray nameBytes = e.name.toUtf8();
        quint32 crcVal = crc32(e.content);
        quint16 nameLen = static_cast<quint16>(nameBytes.size());
        quint32 compSize = e.content.size();
        quint32 uncompSize = e.content.size();

        // Local file header
        out.append("\x50\x4b\x03\x04");
        out.append("\x14\x00");
        out.append("\x00\x00");
        out.append("\x00\x00");
        out.append("\x00\x00");
        out.append("\x00\x00");
        out.append(static_cast<char>(crcVal >> 24));
        out.append(static_cast<char>(crcVal >> 16));
        out.append(static_cast<char>(crcVal >> 8));
        out.append(static_cast<char>(crcVal));
        out.append(static_cast<char>(compSize >> 24));
        out.append(static_cast<char>(compSize >> 16));
        out.append(static_cast<char>(compSize >> 8));
        out.append(static_cast<char>(compSize));
        out.append(static_cast<char>(uncompSize >> 24));
        out.append(static_cast<char>(uncompSize >> 16));
        out.append(static_cast<char>(uncompSize >> 8));
        out.append(static_cast<char>(uncompSize));
        out.append(static_cast<char>(nameLen >> 8));
        out.append(static_cast<char>(nameLen));
        out.append("\x00\x00");
        out.append(nameBytes);
        out.append(e.content);
        offset += 30 + nameLen + compSize;

        // Central directory entry
        central.append("\x50\x4b\x01\x02");
        central.append("\x14\x00");
        central.append("\x14\x00");
        central.append("\x00\x00");
        central.append("\x00\x00");
        central.append("\x00\x00");
        central.append("\x00\x00");
        central.append("\x00\x00");
        central.append("\x00\x00");
        central.append("\x00\x00");
        central.append(static_cast<char>(compSize >> 24));
        central.append(static_cast<char>(compSize >> 16));
        central.append(static_cast<char>(compSize >> 8));
        central.append(static_cast<char>(compSize));
        central.append(static_cast<char>(uncompSize >> 24));
        central.append(static_cast<char>(uncompSize >> 16));
        central.append(static_cast<char>(uncompSize >> 8));
        central.append(static_cast<char>(uncompSize));
        central.append(static_cast<char>(nameLen >> 8));
        central.append(static_cast<char>(nameLen));
        central.append("\x00\x00");
        central.append("\x00\x00");
        central.append("\x00\x00");
        central.append("\x00\x00");
        central.append("\x00\x00\x00\x00");
        central.append(static_cast<char>(offset >> 24));
        central.append(static_cast<char>(offset >> 16));
        central.append(static_cast<char>(offset >> 8));
        central.append(static_cast<char>(offset));
        central.append(nameBytes);
    }

    quint32 cdOffset = offset;
    quint16 cdSize = static_cast<quint16>(central.size());
    quint16 cdEntries = static_cast<quint16>(entries.size());

    // End of central directory
    out.append(central);
    out.append("\x50\x4b\x05\x06");
    out.append("\x00\x00");
    out.append("\x00\x00");
    out.append(static_cast<char>(cdEntries >> 8));
    out.append(static_cast<char>(cdEntries));
    out.append(static_cast<char>(cdSize >> 24));
    out.append(static_cast<char>(cdSize >> 16));
    out.append(static_cast<char>(cdSize >> 8));
    out.append(static_cast<char>(cdSize));
    out.append(static_cast<char>(cdOffset >> 24));
    out.append(static_cast<char>(cdOffset >> 16));
    out.append(static_cast<char>(cdOffset >> 8));
    out.append(static_cast<char>(cdOffset));
    out.append("\x00\x00");

    return out;
}

// ─── Create DOCX ───────────────────────────────────────────────────────────────
static QByteArray createDocx()
{
    QList<ZipEntry> entries = {
        { "[Content_Types].xml",
          "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
          "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
          "<Default Extension=\"xml\" ContentType=\"application/xml\"/>"
          "<Override PartName=\"/word/document.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml\"/>"
          "</Types>" },
        { "_rels/.rels",
          "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
          "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" Target=\"word/document.xml\"/>"
          "</Relationships>" },
        { "word/document.xml",
          "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
          "<w:document xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\">"
          "<w:body><w:p><w:pPr/><w:r><w:t></w:t></w:r></w:p></w:body></w:document>" }
    };
    return buildZip(entries);
}

// ─── Create XLSX ───────────────────────────────────────────────────────────────
static QByteArray createXlsx()
{
    QList<ZipEntry> entries = {
        { "[Content_Types].xml",
          "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
          "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
          "<Default Extension=\"xml\" ContentType=\"application/xml\"/>"
          "<Override PartName=\"/xl/workbook.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml\"/>"
          "<Override PartName=\"/xl/worksheets/sheet1.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml\"/>"
          "</Types>" },
        { "_rels/.rels",
          "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
          "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" Target=\"xl/workbook.xml\"/>"
          "</Relationships>" },
        { "xl/workbook.xml",
          "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
          "<workbook xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">"
          "<sheets><sheet name=\"Sheet1\" sheetId=\"1\" r:id=\"rId1\" xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\"/></sheets></workbook>" },
        { "xl/worksheets/sheet1.xml",
          "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
          "<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">"
          "<sheetData><row r=\"1\"><c r=\"A1\" t=\"str\"><v></v></c></row></sheetData></worksheet>" },
        { "xl/_rels/workbook.xml.rels",
          "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
          "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet\" Target=\"worksheets/sheet1.xml\"/>"
          "</Relationships>" }
    };
    return buildZip(entries);
}

// ─── Create PPTX ───────────────────────────────────────────────────────────────
static QByteArray createPptx()
{
    QList<ZipEntry> entries = {
        { "[Content_Types].xml",
          "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
          "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
          "<Default Extension=\"xml\" ContentType=\"application/xml\"/>"
          "<Override PartName=\"/ppt/presentation.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.presentation.main+xml\"/>"
          "<Override PartName=\"/ppt/slides/slide1.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.slide+xml\"/>"
          "</Types>" },
        { "_rels/.rels",
          "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
          "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" Target=\"ppt/presentation.xml\"/>"
          "</Relationships>" },
        { "ppt/presentation.xml",
          "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
          "<p:presentation xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\">"
          "<p:sldIdLst><p:sldId id=\"256\" r:id=\"rId1\" xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\"/></p:sldIdLst>"
          "<p:sldSz cx=\"9144000\" cy=\"6858000\"/></p:presentation>" },
        { "ppt/slides/slide1.xml",
          "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
          "<p:sld xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\">"
          "<p:cSld><p:spTree>"
          "<p:nvGrpSpPr><p:cNvPr id=\"1\" name=\"\"/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr>"
          "<p:grpSpPr><a:xfrm xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\">"
          "<a:off x=\"0\" y=\"0\"/><a:ext cx=\"0\" cy=\"0\"/><a:chOff x=\"0\" y=\"0\"/><a:chExt cx=\"0\" cy=\"0\"/>"
          "</a:xfrm></p:grpSpPr>"
          "</p:spTree></p:cSld><p:clrMapOvr><a:masterClrMapping/></p:clrMapOvr></p:sld>" },
        { "ppt/_rels/presentation.xml.rels",
          "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
          "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/slide\" Target=\"slides/slide1.xml\"/>"
          "</Relationships>" }
    };
    return buildZip(entries);
}

// ─── Create PNG (1x1 transparent pixel) ────────────────────────────────────────
static QByteArray createPng()
{
    QByteArray out;
    // PNG signature
    out.append("\x89\x50\x4E\x47\x0D\x0A\x1A\x0A");

    // IHDR: 1x1 px, 8-bit RGB
    QByteArray ihdrData;
    ihdrData.append("\x00\x00\x00\x01");  // width=1
    ihdrData.append("\x00\x00\x00\x01");  // height=1
    ihdrData.append("\x08");               // bit depth=8
    ihdrData.append("\x02");               // color type=2 (RGB)
    ihdrData.append("\x00");               // compression=deflate
    ihdrData.append("\x00");               // filter=standard
    ihdrData.append("\x00");               // interlace=none
    out.append(pngChunk("IHDR", ihdrData));

    // IDAT: deflate with filter byte 0 (none) + 3 RGB bytes (black)
    unsigned char rawScanline[4] = { 0, 0, 0, 0 }; // filter=none, R=0,G=0,B=0
    uLongf destLen = compressBound(4);
    QByteArray compressed(destLen, 0);
    if (compress(reinterpret_cast<Bytef*>(compressed.data()), &destLen,
                 reinterpret_cast<const Bytef*>(rawScanline), 4) != Z_OK) {
        return QByteArray();
    }
    compressed.truncate(destLen);
    out.append(pngChunk("IDAT", compressed));

    // IEND
    out.append(pngChunk("IEND", QByteArray()));
    return out;
}

// ─── Create BMP (1x1 white pixel) ──────────────────────────────────────────────
static QByteArray createBmp()
{
    QByteArray bmp;
    bmp.resize(14 + 40 + 4); // header + DIB + 1 pixel + 3-byte row padding

    // BMP File Header (14 bytes)
    bmp[0] = 'B'; bmp[1] = 'M';
    quint32 fileSize = 14 + 40 + 4;
    bmp[2] = static_cast<char>(fileSize);
    bmp[3] = static_cast<char>(fileSize >> 8);
    bmp[4] = static_cast<char>(fileSize >> 16);
    bmp[5] = static_cast<char>(fileSize >> 24);
    bmp[6] = bmp[7] = bmp[8] = bmp[9] = 0;    // reserved
    bmp[10] = 0x36; bmp[11] = 0; bmp[12] = 0; bmp[13] = 0; // pixel offset=54

    // DIB Header BITMAPINFOHEADER (40 bytes)
    bmp[14] = 40; bmp[15] = 0; bmp[16] = 0; bmp[17] = 0; // header size=40
    bmp[18] = 1; bmp[19] = 0;                             // width=1
    bmp[22] = 1; bmp[23] = 0;                             // height=1 (bottom-up)
    bmp[24] = 1; bmp[25] = 0;                             // color planes=1
    bmp[26] = 24;                                         // bits/pixel=24
    bmp[30] = bmp[34] = bmp[38] = 0; bmp[42] = 0;         // image size/xPelsPerMeter/yPelsPerMeter=0

    // Pixel data: 1 white pixel (BGR=FF,FF,FF) + 3 bytes padding per row
    int pixelOffset = 14 + 40;
    bmp[pixelOffset] = 0xFF;     // B
    bmp[pixelOffset + 1] = 0xFF; // G
    bmp[pixelOffset + 2] = 0xFF; // R

    return bmp;
}

// ─── Public API ────────────────────────────────────────────────────────────────
bool NewFileService::create(const QString &fullPath)
{
    QString ext = QFileInfo(fullPath).suffix().toLower();

    QByteArray content;
    if (ext == "txt" || ext == "rtf") {
        content.clear();
    } else if (ext == "docx") {
        content = createDocx();
    } else if (ext == "xlsx") {
        content = createXlsx();
    } else if (ext == "pptx") {
        content = createPptx();
    } else if (ext == "png") {
        content = createPng();
    } else if (ext == "bmp") {
        content = createBmp();
    } else {
        content.clear();
    }

    if (content.isNull())
        return false;

    QDir dir = QFileInfo(fullPath).dir();
    if (!dir.exists())
        dir.mkpath(".");

    QFile f(fullPath);
    if (!f.open(QIODevice::WriteOnly))
        return false;
    qint64 written = f.write(content);
    f.close();
    return written == content.size();
}
