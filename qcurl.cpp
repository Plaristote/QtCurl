#include "qcurl.h"
#include <curl/curl.h>
#include <algorithm>

thread_local QCurl::Reply QCurl::reply;

QCurl::QCurl()
{
  handle = curl_easy_init();
  curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION,  &QCurl::write_data);
  curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, &QCurl::header_callback);
}

QCurl::~QCurl()
{
  if (headers)
    curl_slist_free_all(headers);
  curl_easy_cleanup(handle);
}

void QCurl::setCertificate(const QString& filepath, QSsl::EncodingFormat format)
{
  curl_easy_setopt(handle, CURLOPT_SSLCERTTYPE, format == QSsl::Pem ? "PEM" : "DER");
  curl_easy_setopt(handle, CURLOPT_SSLCERT, filepath.toStdString().c_str());
}

void QCurl::setSslKey(const QString& filepath, QSsl::KeyAlgorithm)
{
  curl_easy_setopt(handle, CURLOPT_SSLKEY, filepath.toStdString().c_str());
}

void QCurl::prepare_headers(const QNetworkRequest& request)
{
  for (const QByteArray& rawHeader : request.rawHeaderList())
  {
    if (rawHeader == "Content-Length") continue ;
    QByteArray headerString = rawHeader + ": " + request.rawHeader(rawHeader);
    headers = curl_slist_append(headers, headerString.data());
  }
}

void QCurl::prepare_body(const QNetworkRequest& request, const QByteArray& body)
{
  std::string content_length = "Content-Length: " + std::to_string(body.length());
  headers = curl_slist_append(headers, content_length.c_str());
  curl_easy_setopt(handle, CURLOPT_POSTFIELDS, body.data());
}

QNetworkReply* QCurl::send(const QNetworkRequest& request, const QByteArray& body)
{
  CURLcode res;
  long status;

  reply.resetReply();
  curl_easy_setopt(handle, CURLOPT_URL, request.url().toString().toStdString().c_str());
  prepare_headers(request);
  if (body.length())
    prepare_body(request, body);
  curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers);
  res = curl_easy_perform(handle);
  if (res == CURLE_OK)
  {
    curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &status);
    reply.setAttribute(QNetworkRequest::HttpStatusCodeAttribute, QVariant::fromValue(status));
    reply.loadHeaders();
    return &reply;
  }
  return nullptr;
}

size_t QCurl::write_data(void *ptr, size_t size, size_t nmemb, struct url_data *data)
{
  reply.writeData(reinterpret_cast<char*>(ptr), size * nmemb);
  return size * nmemb;
}

size_t QCurl::header_callback(char* buffer, size_t size, size_t nitems, void* userdata)
{
  reply.headerData.append(buffer, size * nitems);
  return size * nitems;
}

// Reply
QCurl::Reply::Reply()
{
  open(QIODevice::ReadOnly);
  setFinished(true);
}

qint64 QCurl::Reply::readData(char* out, qint64 maxlen)
{
  if (cursor < data.length())
  {
    qint64 endCursor = std::min(cursor + maxlen, data.length());
    qint64 written = endCursor - cursor;
    std::copy(&data.data()[cursor], &data.data()[endCursor], out);
    cursor = endCursor;
    return written;
  }
  return 0;
}

qint64 QCurl::Reply::writeData(const char *in, qint64 len)
{
  data.append(in, len);
  return len;
}

void QCurl::Reply::loadHeaders()
{
  QList<QByteArray> lines = headerData.split('\n');
  for (const QByteArray& line : lines)
  {
    QByteArray name, value;
    auto separatorAt = line.indexOf(':');
    if (separatorAt > 0)
    {
      std::copy(line.data(), &(line.data()[separatorAt]), std::back_inserter(name));
      std::copy(&(line.data()[separatorAt + 2]), line.end(), std::back_inserter(value));
      value.replace('\r', "");
      setRawHeader(name, value);
    }
  }
}
