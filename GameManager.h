#pragma once

#include <QObject>
#include <QAbstractListModel>
#include <QTimer>
#include <QElapsedTimer>
#include <QSettings>
#include <QRandomGenerator>

// Simple Obstacle struct
struct Obstacle {
    int id;
    double x;
    double y;
    int w;
    int h;
    QString source;
    bool scored;
};

class ObstacleModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles { XRole = Qt::UserRole + 1, YRole, WRole, HRole, SourceRole, IdRole, ScoredRole };
    explicit ObstacleModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void addObstacle(const Obstacle &o);
    void updateObstacle(int row, const Obstacle &o);
    void removeFirst();
    void clear();

    QList<Obstacle> items() const { return m_items; }
    void setItems(const QList<Obstacle>& list);

private:
    QList<Obstacle> m_items;
};

class GameManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int gameState READ gameState NOTIFY gameStateChanged)
    Q_PROPERTY(int score READ score NOTIFY scoreChanged)
    Q_PROPERTY(int highScore READ highScore NOTIFY highScoreChanged)
    Q_PROPERTY(int difficulty READ difficulty WRITE setDifficulty NOTIFY difficultyChanged)
    Q_PROPERTY(double playerY READ playerY NOTIFY playerYChanged)
    Q_PROPERTY(QAbstractListModel* obstacleModel READ obstacleModel CONSTANT)

public:
    explicit GameManager(QObject *parent = nullptr);
    ~GameManager() override = default;

    int gameState() const { return m_gameState; }
    int score() const { return m_score; }
    int highScore() const { return m_highScore; }
    int difficulty() const { return m_difficulty; }
    double playerY() const { return m_playerY; }

    Q_INVOKABLE void startGame();
    Q_INVOKABLE void triggerJump();
    Q_INVOKABLE void pauseGame();
    Q_INVOKABLE void resumeGame();
    Q_INVOKABLE void finishedGame();

    Q_INVOKABLE void setDifficulty(int d);

    QAbstractListModel* obstacleModel() { return &m_obstacles; }

signals:
    void gameStateChanged();
    void scoreChanged();
    void highScoreChanged();
    void difficultyChanged();
    void playerYChanged();

    // audio requests to be handled by QML
    void playJumpSound();
    void playHitSound();
    void playBgm();
    void pauseBgm();

private slots:
    void onTick();

private:
    void advance(qreal dt);
    void spawnObstacleIfNeeded();
    bool checkCollisionWith(const Obstacle &o);

private:
    int m_gameState = 0; // 0 Ready,1 Running,2 Paused,3 GameOver
    int m_score = 0;
    int m_highScore = 0;
    int m_difficulty = 1;
    double m_playerY = 220.0;
    double m_playerVy = 0.0;

    QTimer m_timer;
    QElapsedTimer m_elapsed;
    ObstacleModel m_obstacles;

    int m_nextObstacleId = 1;

    const double GRAVITY = 2000.0;     // px/s^2
    const double JUMP_VELOCITY = -680.0; // px/s
    const double GROUND_Y = 220.0;
};