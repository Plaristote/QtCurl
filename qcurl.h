#ifndef  QCURL_HPP
# define QCURL_HPP

# include <QNetworkReply>
# include <QNetworkRequest>
# include <QSslConfiguration>
# include <QSslKey>

struct curl_list;

class QCurl
{
  struct Reply : public QNetworkReply
  {
    friend class QCurl;

    Reply();

    void resetReply() { cursor = 0; data = ""; }

    qint64 readData(char* out, qint64 maxlen) override;
    qint64 writeData(const char *in, qint64 len) override;
    void abort() override {}
    void loadHeaders();

  private:
    qint64 cursor = 0;
    QByteArray data;
    QByteArray headerData;
  };

public:
  QCurl();
  QCurl(const QCurl&) = delete;
  ~QCurl();

  QNetworkReply* send(const QNetworkRequest& request, const QByteArray& body = "");
  void setCertificate(const QString& filePath, QSsl::EncodingFormat format = QSsl::Pem);
  void setSslKey(const QString& filePath, QSsl::KeyAlgorithm algorithm = QSsl::Rsa);
  void setVerbosityLevel(unsigned int);

  void* operator*() { return handle; }

private:
  void prepare_headers(const QNetworkRequest&);
  void prepare_body(const QNetworkRequest&, const QByteArray& body);
  static size_t write_data(void *ptr, size_t size, size_t nmemb, void* data);
  static size_t header_callback(char* buffer, size_t size, size_t nitems, void* userdata);
  void handle_success(Reply&);
  void handle_failure(Reply&, void*);

  void* handle;
  struct curl_slist* headers = nullptr;
  static thread_local Reply reply;
};

#endif
