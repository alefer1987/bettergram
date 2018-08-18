#include "rsschannel.h"
#include "rssitem.h"
#include <logs.h>
#include <QDateTime>
#include <QCryptographicHash>
#include <QXmlStreamReader>

namespace Bettergram {

void RssChannel::sort(QList<QSharedPointer<RssItem>> &items)
{
	std::sort(items.begin(), items.end(), &RssChannel::compare);
}

bool RssChannel::compare(const QSharedPointer<RssItem> &a, const QSharedPointer<RssItem> &b)
{
	return a->publishDate() > b->publishDate();
}

RssChannel::RssChannel(QObject *parent) :
	QObject(parent)
{
}

RssChannel::RssChannel(const QUrl &feedLink, QObject *parent) :
	QObject(parent),
	_feedLink(feedLink)
{
}

const QString &RssChannel::title() const
{
	return _title;
}

void RssChannel::setTitle(const QString &title)
{
	_title = title;
}

const QString &RssChannel::description() const
{
	return _description;
}

void RssChannel::setDescription(const QString &description)
{
	_description = description;
}

const QString &RssChannel::language() const
{
	return _language;
}

void RssChannel::setLanguage(const QString &language)
{
	_language = language;
}

const QString &RssChannel::copyright() const
{
	return _copyright;
}

void RssChannel::setCopyright(const QString &copyright)
{
	_copyright = copyright;
}

const QString &RssChannel::editorEmail() const
{
	return _editorEmail;
}

void RssChannel::setEditorEmail(const QString &editorEmail)
{
	_editorEmail = editorEmail;
}

const QString &RssChannel::webMasterEmail() const
{
	return _webMasterEmail;
}

void RssChannel::setWebMasterEmail(const QString &webMasterEmail)
{
	_webMasterEmail = webMasterEmail;
}

const QStringList &RssChannel::categoryList() const
{
	return _categoryList;
}

void RssChannel::setCategoryList(const QStringList &categoryList)
{
	_categoryList = categoryList;
}

const QDateTime &RssChannel::publishDate() const
{
	return _publishDate;
}

void RssChannel::setPublishDate(const QDateTime &publishDate)
{
	_publishDate = publishDate;
}

const QDateTime &RssChannel::lastBuildDate() const
{
	return _lastBuildDate;
}

void RssChannel::setLastBuildDate(const QDateTime &lastBuildDate)
{
	_lastBuildDate = lastBuildDate;
}

const QString &RssChannel::skipHours() const
{
	return _skipHours;
}

void RssChannel::setSkipHours(const QString &skipHours)
{
	_skipHours = skipHours;
}

const QString &RssChannel::skipDays() const
{
	return _skipDays;
}

void RssChannel::setSkipDays(const QString &skipDays)
{
	_skipDays = skipDays;
}
const QUrl &RssChannel::feedLink() const
{
	return _feedLink;
}

void RssChannel::setFeedLink(const QUrl &feedLink)
{
	_feedLink = feedLink;
}

const QUrl &RssChannel::link() const
{
	return _link;
}

void RssChannel::setLink(const QUrl &link)
{
	_link = link;
}

const QUrl &RssChannel::image() const
{
	return _image;
}

void RssChannel::setImage(const QUrl &image)
{
	_image = image;
}

bool RssChannel::isFetching() const
{
	return _isFetching;
}

void RssChannel::setIsFetching(bool isFetching)
{
	_isFetching = isFetching;
}

bool RssChannel::isFailed() const
{
	return _isFailed;
}

void RssChannel::setIsFailed(bool isFailed)
{
	_isFailed = isFailed;
}

QByteArray RssChannel::countSourceHash(const QByteArray &source) const
{
	return QCryptographicHash::hash(source, QCryptographicHash::Sha256);
}

RssChannel::const_iterator RssChannel::begin() const
{
	return _list.begin();
}

RssChannel::const_iterator RssChannel::end() const
{
	return _list.end();
}

const QSharedPointer<RssItem> &RssChannel::at(int index) const
{
	if (index < 0 || index >= _list.size()) {
		throw std::out_of_range("Unable to get RssItem at wrong index");
	}

	return _list.at(index);
}

const QList<QSharedPointer<RssItem>> &RssChannel::getAllItems() const
{
	return _list;
}

QList<QSharedPointer<RssItem>> RssChannel::getAllUnreadItems() const
{
	QList<QSharedPointer<RssItem>> result;

	for (const QSharedPointer<RssItem> &item : _list) {
		if (!item->isRead()) {
			result.push_back(item);
		}
	}

	return result;
}

int RssChannel::count() const
{
	return _list.count();
}

int RssChannel::countUnread() const
{
	int result = 0;

	for (const QSharedPointer<RssItem> &item : _list) {
		if (item->isRead()) {
			result++;
		}
	}

	return  result;
}

bool RssChannel::isMayFetchNewData() const
{
	return !_isFetching;
}

void RssChannel::startFetching()
{
	setIsFetching(true);
}

void RssChannel::fetchingSucceed(const QByteArray &source)
{
	qDebug() << QString("fetching succeed for %1").arg(_feedLink.toString());

	// Update source only if it has been changed
	if (countSourceHash(source) != _lastSourceHash) {
		_source = source;
	}

	setIsFetching(false);
	setIsFailed(false);
}

void RssChannel::fetchingFailed()
{
	qDebug() << QString("fetching failed for %1").arg(_feedLink.toString());
	_source.clear();
	setIsFetching(false);
	setIsFailed(true);
}

bool RssChannel::parse()
{
	qDebug() << QString("parsing for %1").arg(_feedLink.toString());

	if (_source.isEmpty()) {
		return false;
	}

	_categoryList.clear();

	QXmlStreamReader xml;
	xml.addData(_source);

	while (xml.readNextStartElement()) {
		if (xml.name() == QLatin1String("rss")) {
			parseRss(xml);
		} else {
			xml.skipCurrentElement();
		}
	}

	// readNextStartElement() does not handle end of a document correctly,
	// so we ignore PrematureEndOfDocumentError
	if (xml.hasError() && xml.error() != QXmlStreamReader::PrematureEndOfDocumentError) {
		LOG(("Unable to parse RSS feed from %1. %2 (%3)")
			.arg(_feedLink.toString())
			.arg(xml.errorString())
			.arg(xml.error()));
	}

	_lastSourceHash = countSourceHash(_source);
	_source.clear();

	sort(_list);

	return true;
}

void RssChannel::parseRss(QXmlStreamReader &xml)
{
	while (xml.readNextStartElement()) {
		if (xml.name() == QLatin1String("channel")) {
			parseChannel(xml);
		} else {
			xml.skipCurrentElement();
		}
	}
}

void RssChannel::parseChannel(QXmlStreamReader &xml)
{
	while (xml.readNextStartElement()) {
		if (xml.name() == QLatin1String("item")) {
			parseItem(xml);
		} else if (xml.name() == QLatin1String("title")) {
			setTitle(xml.readElementText());
		} else if (xml.name() == QLatin1String("link")) {
			setLink(xml.readElementText());
		} else if (xml.name() == QLatin1String("description")) {
			setDescription(xml.readElementText());
		} else if (xml.name() == QLatin1String("image")) {
			parseChannelImage(xml);
		} else if (xml.name() == QLatin1String("language")) {
			setLanguage(xml.readElementText());
		} else if (xml.name() == QLatin1String("copyright")) {
			setCopyright(xml.readElementText());
		} else if (xml.name() == QLatin1String("managingEditor")) {
			setEditorEmail(xml.readElementText());
		} else if (xml.name() == QLatin1String("webmaster")) {
			setWebMasterEmail(xml.readElementText());
		} else if (xml.name() == QLatin1String("pubDate")) {
			// Please note that thid property may not exist
			setPublishDate(QDateTime::fromString(xml.readElementText(), Qt::RFC2822Date));
		} else if (xml.name() == QLatin1String("lastBuildDate")) {
			setLastBuildDate(QDateTime::fromString(xml.readElementText(), Qt::RFC2822Date));
		} else if (xml.name() == QLatin1String("skipHours")) {
			setSkipHours(xml.readElementText());
		} else if (xml.name() == QLatin1String("skipDays")) {
			setSkipDays(xml.readElementText());
		} else if (xml.name() == QLatin1String("category")) {
			_categoryList.push_back(xml.readElementText());
		} else {
			xml.skipCurrentElement();
		}
	}
}

void RssChannel::parseChannelImage(QXmlStreamReader &xml)
{
	while (xml.readNextStartElement()) {
		if (xml.name() == QLatin1String("url")) {
			setImage(xml.readElementText());
		} else {
			xml.skipCurrentElement();
		}
	}
}

void RssChannel::parseItem(QXmlStreamReader &xml)
{
	QSharedPointer<RssItem> item(new RssItem(this));
	item->parseItem(xml);

	if (xml.hasError()) {
		LOG(("Unable to parse RSS feed item from %1. %2 (%3)")
			.arg(_feedLink.toString())
			.arg(xml.errorString())
			.arg(xml.error()));

		item->deleteLater();
	} else {
		merge(item);
	}
}

void RssChannel::merge(const QSharedPointer<RssItem> &item)
{
	//TODO: bettergram: realize RssChannel::merge() method

	_list.push_back(item);
}

} // namespace Bettergrams