/*
 * Copyright (C) 2026 LingmoOS Team.
 *
 * Author:     devalexandre <alexandre@dev2learn.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <QFutureWatcher>
#include <QObject>
#include <QSettings>
#include <QTimer>

#include <KService>

#include "contentsearch.h"
#include "currencyrates.h"
#include "resultsmodel.h"

class IntentProvider;

class SearchEngine : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY queryChanged)
    Q_PROPERTY(ResultsModel *results READ results CONSTANT)

public:
    explicit SearchEngine(QObject *parent = nullptr);

    QString query() const { return m_query; }
    void setQuery(const QString &query);
    ResultsModel *results() { return &m_model; }

    // Runs the result's action; returns true when the popup should close
    Q_INVOKABLE bool activate(int row);
    // App actions (Lingmo Intents); null when built without liblingmo-intents
    void setIntentProvider(IntentProvider *provider) { m_intents = provider; }

    // A question ("…?") goes to the AI chat instead of the web
    Q_INVOKABLE static bool isQuestion(const QString &text);

Q_SIGNALS:
    void queryChanged();
    // The user picked "Ask the AI": open the chat with this question
    void chatRequested(const QString &question);

private:
    void reloadApps();
    void search();
    void rebuild();
    QList<SearchResult> instantResults(const QString &text);
    void startFileSearch(const QString &query, quint64 generation);

    QList<SearchResult> calculator(const QString &q) const;
    QList<SearchResult> aiAssistant(const QString &q) const;
    QList<SearchResult> conversions(const QString &q);
    QList<SearchResult> apps(const QString &q) const;
    QList<SearchResult> settings(const QString &q) const;
    // Lingmo Assistant workflows with a manual trigger shown in Spotlight
    QList<SearchResult> workflows(const QString &q) const;
    QList<SearchResult> intents(const QString &text) const;

    QString m_query;
    ResultsModel m_model;
    QList<KService::Ptr> m_apps;
    QSettings m_usage;

    // Results shown for the current query: the instant ones, then files and
    // file contents as they arrive, then "Search the web"
    QList<SearchResult> m_current;
    QList<SearchResult> m_files;
    QList<SearchResult> m_contents;
    QList<SearchResult> m_web;
    bool m_searchContents = false;
    CurrencyRates m_rates;
    ContentSearch m_contentSearch;
    quint64 m_generation = 0;
    QFutureWatcher<QList<SearchResult>> m_fileWatcher;
    QTimer m_fileDebounce;
    IntentProvider *m_intents = nullptr;
};
