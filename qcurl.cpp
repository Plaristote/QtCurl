#include "qcurl.h"
#include <curl/curl.h>
#include <algorithm>

thread_local QCurl::Reply QCurl::reply;

QMap<int, QNetworkReply::NetworkError> errorMap{
  {CURLE_OK,                    QNetworkReply::NoError},
  {CURLE_UNSUPPORTED_PROTOCOL,  QNetworkReply::ProtocolUnknownError},
  {CURLE_HTTP2,                 QNetworkReply::ProtocolFailure},
  {CURLE_HTTP3,                 QNetworkReply::ProtocolFailure},
  {CURLE_OPERATION_TIMEDOUT,    QNetworkReply::TimeoutError},
  {CURLE_COULDNT_CONNECT,       QNetworkReply::ConnectionRefusedError},
  {CURLE_SSL_CONNECT_ERROR,     QNetworkReply::SslHandshakeFailedError},
  {CURLE_TOO_MANY_REDIRECTS,    QNetworkReply::TooManyRedirectsError},
  {CURLE_SEND_ERROR,            QNetworkReply::UnknownNetworkError},
  {CURLE_RECV_ERROR,            QNetworkReply::UnknownNetworkError},
  {CURLE_COULDNT_RESOLVE_HOST,  QNetworkReply::HostNotFoundError},
  {CURLE_COULDNT_RESOLVE_PROXY, QNetworkReply::ProxyNotFoundError},
  {CURLE_WEIRD_SERVER_REPLY,    QNetworkReply::UnknownContentError},
  {CURLE_RANGE_ERROR,           QNetworkReply::ContentOperationNotPermittedError},
  {CURLE_GOT_NOTHING,           QNetworkReply::ContentNotFoundError},
  {CURLE_REMOTE_ACCESS_DENIED,  QNetworkReply::ContentAccessDenied},
  {CURLE_PARTIAL_FILE,          QNetworkReply::ContentReSendError},
  {CURLE_WRITE_ERROR,           QNetworkReply::UnknownContentError},
  {CURLE_READ_ERROR,            QNetworkReply::UnknownContentError},
  {CURLE_OUT_OF_MEMORY,         QNetworkReply::UnknownContentError},
  {CURLE_BAD_DOWNLOAD_RESUME,   QNetworkReply::UnknownContentError},
  {CURLE_FILE_COULDNT_READ_FILE,QNetworkReply::UnknownContentError},
  {CURLE_HTTP_POST_ERROR,       QNetworkReply::UnknownContentError}
};

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

void QCurl::handle_success()
{
  long status;

  curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &status);
  reply.setAttribute(QNetworkRequest::HttpStatusCodeAttribute, QVariant::fromValue(status));
  reply.loadHeaders();
}

void QCurl::handle_failure(void* resptr)
{
  CURLcode& res = *reinterpret_cast<CURLcode*>(resptr);

  QNetworkReply::NetworkError errorCode = errorMap.find(res) != errorMap.end()
    ? errorMap[res]
    : QNetworkReply::UnknownNetworkError;

  reply.setError(errorCode, curl_easy_strerror(res));
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
    handle_success();
  else
    handle_failure(&res);
  return &reply;
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
    qint64 endCursor = std::min<qint64>(cursor + maxlen, data.length());
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
