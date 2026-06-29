#include "musicinfo.h"

#include "taglib.h"

#include "fileref.h"
#include "mpeg/mpegfile.h"
#include "mpeg/id3v2/id3v2tag.h"

#include "flac/flacfile.h"
#include "flac/flacpicture.h"
#include "mpeg/id3v2/frames/attachedpictureframe.h"

#include "mp4/mp4file.h"
#include "ape/apefile.h"
#include "wavpack/wavpackfile.h"
#include "trueaudio/trueaudiofile.h"
#include "riff/wav/wavfile.h"
#include "riff/aiff/aifffile.h"
#include "ogg/vorbis/vorbisfile.h"
#include "ogg/opus/opusfile.h"
#include "ogg/flac/oggflacfile.h"

#include "taglib_utils.h"



MusicInfo::MusicInfo() {}

void MusicInfo::extraMusicInfo()
{
    try {
        TagLib::FileRef fileRef(TaglibUtils::createTaglibFileRef(musicFilePath));

        if (fileRef.isNull() or not fileRef.file())
        {
            return;
        }

        TagLib::Tag* tag = fileRef.tag();
        if (tag)
        {
            metadata.title = TaglibUtils::tagStringToQString(tag->title());
            metadata.artist = TaglibUtils::tagStringToQString((tag->artist()));
            metadata.album = TaglibUtils::tagStringToQString((tag->album()));
            metadata.genre = TaglibUtils::tagStringToQString((tag->genre()));
            metadata.year = tag->year();
            metadata.track = tag->track();
            metadata.comment = TaglibUtils::tagStringToQString((tag->comment()));
        }


        TagLib::AudioProperties* audioProps = fileRef.audioProperties();
        if (audioProps) {
            metadata.lengthInSeconds= audioProps->lengthInSeconds();
            metadata.bitrate = audioProps->bitrate();
            metadata.sampleRate = audioProps->sampleRate();
            metadata.channels = audioProps->channels();
        }

        // 通过TagLib魔术字节检测真实音频格式（不依赖文件后缀）
        TagLib::File* f = fileRef.file();
        if (dynamic_cast<TagLib::MPEG::File*>(f)) {
            metadata.audioFormat = "MP3";
        } else if (dynamic_cast<TagLib::FLAC::File*>(f)) {
            metadata.audioFormat = "FLAC";
        } else if (dynamic_cast<TagLib::RIFF::WAV::File*>(f)) {
            metadata.audioFormat = "WAV";
        } else if (dynamic_cast<TagLib::RIFF::AIFF::File*>(f)) {
            metadata.audioFormat = "AIFF";
        } else if (dynamic_cast<TagLib::APE::File*>(f)) {
            metadata.audioFormat = "APE";
        } else if (dynamic_cast<TagLib::WavPack::File*>(f)) {
            metadata.audioFormat = "WavPack";
        } else if (dynamic_cast<TagLib::TrueAudio::File*>(f)) {
            metadata.audioFormat = "TTA";
        } else if (dynamic_cast<TagLib::MP4::File*>(f)) {
            metadata.audioFormat = "M4A";
        } else if (dynamic_cast<TagLib::Ogg::FLAC::File*>(f)) {
            metadata.audioFormat = "OggFLAC";
        } else if (dynamic_cast<TagLib::Ogg::Vorbis::File*>(f)) {
            metadata.audioFormat = "OGG";
        } else if (dynamic_cast<TagLib::Ogg::Opus::File*>(f)) {
            metadata.audioFormat = "Opus";
        }

        this->cover = extraMusicCover(fileRef);

    }  catch (const std::exception& e) {
        qWarning() << "提取信息失败：" << e.what();
    }
}


inline static QImage convertTaglibDataToQImage(const TagLib::ByteVector& data) {
    if (data.isEmpty()) {
        return QImage();
    }

    QByteArray imageData(reinterpret_cast<const char*>(data.data()), data.size());
    QImage coverImage;
    coverImage.loadFromData(imageData);
    return coverImage;
}


QImage MusicInfo::extraMusicCover(const TagLib::FileRef& fileRef)
{
    QImage coverImage;

    try {

        if (fileRef.isNull() || !fileRef.file()) {
            qWarning() << "提取封面失败：无法打开文件" << musicFilePath;
            return coverImage;
        }

        TagLib::MPEG::File* mpegFile = dynamic_cast<TagLib::MPEG::File*>(fileRef.file());
        if (mpegFile) {
            TagLib::ID3v2::Tag* id3v2Tag = mpegFile->ID3v2Tag();
            if (!id3v2Tag) {
                qDebug() << "MP3文件无ID3v2标签：" << musicFilePath;
            } else {
                TagLib::ID3v2::FrameList frameList = id3v2Tag->frameList("APIC");
                if (frameList.isEmpty()) {
                    qDebug() << "MP3文件无APIC封面帧：" << musicFilePath;
                } else {
                    for (auto it = frameList.begin(); it != frameList.end(); ++it) {
                        TagLib::ID3v2::AttachedPictureFrame* picFrame =
                            dynamic_cast<TagLib::ID3v2::AttachedPictureFrame*>(*it);
                        if (!picFrame || picFrame->picture().isEmpty()) {
                            continue;
                        }
                        coverImage = convertTaglibDataToQImage(picFrame->picture());
                        if (!coverImage.isNull()) {
                            qDebug() << "成功提取MP3封面：" << musicFilePath;
                            return coverImage;
                        }
                    }
                }
            }
        }

        TagLib::FLAC::File* flacFile = dynamic_cast<TagLib::FLAC::File*>(fileRef.file());
        if (flacFile) {
            const TagLib::List<TagLib::FLAC::Picture*>& picList = flacFile->pictureList();
            if (picList.isEmpty()) {
                qDebug() << "FLAC文件无Picture封面帧：" << musicFilePath;
            } else {
                for (auto it = picList.begin(); it != picList.end(); ++it) {
                    TagLib::FLAC::Picture* pic = *it;
                    if (!pic || pic->data().isEmpty()) {
                        continue;
                    }
                    coverImage = convertTaglibDataToQImage(pic->data());
                    if (!coverImage.isNull()) {
                        qDebug() << "成功提取FLAC封面：" << musicFilePath;
                        return coverImage; // 找到有效封面立即返回
                    }
                }
            }
        }

          qDebug() << "文件无内置封面：" << musicFilePath;
    }  catch (const std::exception& e) {
        qWarning() << "提取封面异常：" << e.what() << "，文件：" << musicFilePath;
    } catch (...) {
        qWarning() << "提取封面未知异常，文件：" << musicFilePath;
    }

    return coverImage;
}
