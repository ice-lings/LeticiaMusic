#ifndef DEDUP_ENGINE_H
#define DEDUP_ENGINE_H

#include <QString>
#include <QVariantMap>
#include <QList>
#include <QPair>
#include <QSet>

class DedupEngine
{
public:
    // 标题归一化（严格）：仅移除 feat. 括号，保留版本标识
    // "Cocoon(茧缚)" → "cocoon(茧缚)"，与 "Cocoon" → "cocoon" 区分
    static QString normalizeTitle(const QString& title);

    // 标题归一化（宽松）：移除所有括号内容，仅保留核心歌名
    // "Cocoon(茧缚)" → "cocoon"，与 "Cocoon" → "cocoon" 合并
    static QString normalizeTitleRelaxed(const QString& title);

    // 艺术家归一化：去空格、去标点、合并分隔符（&/,/feat.）、转小写
    static QString normalizeArtist(const QString& artist);

    // 判断是否为同一首歌（严格）：标题归一化匹配 + 艺术家归一化匹配 + 时长差 ≤ 5 秒
    static bool isSameSong(const QString& t1, const QString& a1, int dur1,
                           const QString& t2, const QString& a2, int dur2);

    // 计算音质分数（越高越好）
    static int qualityScore(const QString& qualityLabel, int bitrate, const QString& audioFormat);

    // 比较 a 是否比 b 音质更高
    static bool isHigherQuality(const QString& qa, int ba, const QString& fa,
                                const QString& qb, int bb, const QString& fb);

    // 重复组数据
    struct DuplicateGroup {
        QString groupKey;
        QString displayTitle;
        QString displayArtist;
        QList<QVariantMap> entries;
    };

    // 构建重复组（宽松匹配）：
    //   按 normalizeTitleRelaxed 分组，不限艺术家，时长差 ≤ 30s
    //   resolvedHashes 中的条目会被跳过
    static QList<DuplicateGroup> buildDuplicateGroups(
        const QList<class MusicItem>& musicList,
        const QSet<QString>& resolvedHashes = {});
};

#endif // DEDUP_ENGINE_H
