#include "dedup_engine.h"
#include "music_entity_type.h"
#include <QRegularExpression>
#include <QMap>
#include <cmath>

QString DedupEngine::normalizeTitle(const QString& title)
{
    QString t = title.trimmed().toLower();

    // 仅移除 feat./ft./featuring 相关的括号内容，保留版本标识
    // 如 (Instrumental)、(Remix)、(Live)、(Acoustic) 等不会被移除
    static const QRegularExpression featBracketRx(
        R"(\(feat\.?[^)]*\)|\[feat\.?[^\]]*\])",
        QRegularExpression::CaseInsensitiveOption);
    t.remove(featBracketRx);

    // 移除 "feat." "ft." "featuring" 及其后文本（无括号形式）
    static const QRegularExpression featRx(
        R"(\b(feat\.?|ft\.?|featuring)\s.*)",
        QRegularExpression::CaseInsensitiveOption);
    t.remove(featRx);

    // 移除多余空格和标点
    static const QRegularExpression punctRx(R"([\s\-_.,;:'"!?]+)");
    t.replace(punctRx, "");

    return t;
}

QString DedupEngine::normalizeTitleRelaxed(const QString& title)
{
    QString t = title.trimmed().toLower();

    // 移除所有括号内容：(feat.xxx) (Instrumental) (Remix) (茧缚) 等
    static const QRegularExpression allBrackets(R"(\([^)]*\)|\[[^\]]*\])");
    t.remove(allBrackets);

    // 移除 feat./ft. 及其后文本
    static const QRegularExpression featRx(
        R"(\b(feat\.?|ft\.?|featuring)\s.*)",
        QRegularExpression::CaseInsensitiveOption);
    t.remove(featRx);

    // 合并空白
    t = t.simplified();
    return t;
}

QString DedupEngine::normalizeArtist(const QString& artist)
{
    QString a = artist.trimmed().toLower();

    // 统一分隔符 & → ""
    static const QRegularExpression sepRx(R"(\s*[&,、/·]\s*)");
    a.replace(sepRx, "");

    // 移除 "feat." "ft." 及其后文本
    static const QRegularExpression featRx(R"(\b(feat\.?|ft\.?|featuring)\s.*)", QRegularExpression::CaseInsensitiveOption);
    a.remove(featRx);

    // 移除多余空格和标点
    static const QRegularExpression punctRx(R"([\s\-_.,;:'"!?]+)");
    a.replace(punctRx, "");

    return a;
}

bool DedupEngine::isSameSong(const QString& t1, const QString& a1, int dur1,
                              const QString& t2, const QString& a2, int dur2)
{
    if (normalizeTitle(t1) != normalizeTitle(t2))
        return false;

    if (normalizeArtist(a1) != normalizeArtist(a2))
        return false;

    // 时长差 ≤ 5 秒
    if (std::abs(dur1 - dur2) > 5)
        return false;

    return true;
}

int DedupEngine::qualityScore(const QString& qualityLabel, int bitrate, const QString& audioFormat)
{
    int score = 0;

    // 无损格式基础分
    const QString fmt = audioFormat.toUpper();
    if (fmt == "FLAC" || fmt == "OGGFLAC") score = 10000;
    else if (fmt == "WAV" || fmt == "AIFF")     score = 9000;
    else if (fmt == "APE" || fmt == "WAVPACK")  score = 8000;
    else if (fmt == "TTA")                      score = 7000;
    else if (fmt == "ALAC")                     score = 6000;
    else                                        score = 0;  // 有损

    // 音质等级加分
    if (qualityLabel == "HI")       score += 4000;
    else if (qualityLabel == "SQ")  score += 3000;
    else if (qualityLabel == "HQ")  score += 2000;
    else if (qualityLabel == "SD")  score += 1000;
    else if (qualityLabel == "LQ")  score += 500;

    // 码率加分（kbps 直接加）
    score += bitrate;

    return score;
}

bool DedupEngine::isHigherQuality(const QString& qa, int ba, const QString& fa,
                                   const QString& qb, int bb, const QString& fb)
{
    return qualityScore(qa, ba, fa) > qualityScore(qb, bb, fb);
}

QList<DedupEngine::DuplicateGroup> DedupEngine::buildDuplicateGroups(
    const QList<MusicItem>& musicList, const QSet<QString>& resolvedHashes)
{
    // 按宽松归一化标题分组（忽略艺术家，不限艺术家）
    QMap<QString, DuplicateGroup> groups;

    for (const auto& item : musicList) {
        // 跳过已处理的条目
        if (resolvedHashes.contains(item.fileHash))
            continue;

        const QString key = normalizeTitleRelaxed(item.title);

        if (!groups.contains(key)) {
            DuplicateGroup g;
            g.groupKey = key;
            g.displayTitle = item.title;
            g.displayArtist = item.artist;
            groups[key] = g;
        }

        QVariantMap entry;
        entry["fileHash"] = item.fileHash;
        entry["title"] = item.title;
        entry["artist"] = item.artist;
        entry["duration"] = item.duration;
        entry["quality"] = item.quality;
        entry["filePath"] = item.filePath;
        entry["coverPath"] = item.coverPath;
        groups[key].entries.append(entry);
    }

    QList<DuplicateGroup> result;
    for (auto it = groups.begin(); it != groups.end(); ++it) {
        if (it.value().entries.size() < 2)
            continue;

        // 按时长分组：时长差 > 30s 的可能是不同版本，拆成不同子组
        QList<QVariantMap>& entries = it.value().entries;

        QList<QList<QVariantMap>> subGroups;
        for (const auto& entry : entries) {
            QString durStr = entry["duration"].toString();
            QStringList parts = durStr.split(":");
            int durSec = 0;
            if (parts.size() == 2) {
                durSec = parts[0].toInt() * 60 + parts[1].toInt();
            }

            bool added = false;
            for (auto& sg : subGroups) {
                QString refDur = sg.first()["duration"].toString();
                QStringList refParts = refDur.split(":");
                int refSec = 0;
                if (refParts.size() == 2) {
                    refSec = refParts[0].toInt() * 60 + refParts[1].toInt();
                }
                if (std::abs(durSec - refSec) <= 30) {
                    sg.append(entry);
                    added = true;
                    break;
                }
            }
            if (!added) {
                subGroups.append({entry});
            }
        }

        // 每个子组 ≥2 条才是重复
        for (const auto& sg : subGroups) {
            if (sg.size() >= 2) {
                DuplicateGroup dg;
                dg.groupKey = it.value().groupKey;
                dg.displayTitle = sg.first()["title"].toString();
                dg.displayArtist = sg.first()["artist"].toString();
                dg.entries = sg;
                result.append(dg);
            }
        }
    }

    return result;
}
