#include <QBuffer>
#include <QDataStream>
#include <QDir>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QProcess>
#include <QSaveFile>
#include <QSvgRenderer>
#include <QTemporaryDir>

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    if (argc != 3) qFatal("Usage: brand-icons source.svg output-directory");
    QSvgRenderer svg(QString::fromLocal8Bit(argv[1]));
    if (!svg.isValid()) qFatal("Invalid icon source");
    const QDir output(QString::fromLocal8Bit(argv[2]));
    if (!QDir().mkpath(output.absolutePath())) qFatal("Cannot create output directory");
    auto render = [&](int size) {
        QImage image(size, size, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        {
            QPainter painter(&image);
            painter.setRenderHint(QPainter::Antialiasing);
            svg.render(&painter);
        }
        return image;
    };
    if (!render(1024).save(output.filePath("sunshine.png"))) qFatal("Cannot write PNG");

    const QList<int> sizes{16, 20, 24, 32, 40, 48, 64, 128, 256};
    QList<QByteArray> frames;
    for (const int size : sizes) {
        QByteArray bytes;
        QBuffer buffer(&bytes);
        if (!buffer.open(QIODevice::WriteOnly) || !render(size).save(&buffer, "PNG")) qFatal("Cannot encode ICO frame");
        frames.append(bytes);
    }
    // Windows supports PNG-compressed frames; each size is rasterized directly
    // from the vector master so shell icons do not upscale the tray bitmap.
    QSaveFile ico(output.filePath("sunshine.ico"));
    if (!ico.open(QIODevice::WriteOnly)) qFatal("Cannot open ICO");
    QDataStream stream(&ico);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream << quint16(0) << quint16(1) << quint16(sizes.size());
    quint32 offset = 6 + 16 * sizes.size();
    for (int i = 0; i < sizes.size(); ++i) {
        const auto dimension = quint8(sizes[i] == 256 ? 0 : sizes[i]);
        stream << dimension << dimension << quint8(0) << quint8(0) << quint16(1) << quint16(32)
               << quint32(frames[i].size()) << offset;
        offset += frames[i].size();
    }
    for (const auto &bytes : frames)
        if (ico.write(bytes) != bytes.size()) qFatal("Cannot write ICO frame");
    if (stream.status() != QDataStream::Ok || !ico.commit()) qFatal("Cannot finish ICO");

#ifdef Q_OS_MACOS
    QTemporaryDir temporary;
    if (!temporary.isValid()) qFatal("Cannot create temporary directory");
    const QDir iconset(temporary.path() + "/sunshine.iconset");
    if (!QDir().mkpath(iconset.absolutePath())) qFatal("Cannot create iconset");
    for (const int size : {16, 32, 128, 256, 512}) {
        for (const int scale : {1, 2}) {
            const auto name = QString("icon_%1x%1%2.png").arg(size).arg(scale == 2 ? "@2x" : "");
            if (!render(size * scale).save(iconset.filePath(name))) qFatal("Cannot write iconset frame");
        }
    }
    if (QProcess::execute("iconutil", {"-c", "icns", "-o", output.filePath("sunshine.icns"), iconset.absolutePath()}) != 0)
        qFatal("iconutil failed");
#endif
    qInfo("Exported Sunshine platform icons");
}
