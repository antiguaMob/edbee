/**
 * Copyright 2011-2013 - Reliable Bits Software by Blommers IT. All Rights Reserved.
 * Author Rick Blommers
 */

#include "textgrammar.h"

#include <QDir>

#include "edbee/io/tmlanguageparser.h"
#include "edbee/util/regexp.h"

#include "debug.h"

namespace edbee {


/// The text grammar rule constructor
/// @param grammar the grammar this rule belongs to
/// @param instruction the type of instruction this is
TextGrammarRule::TextGrammarRule(TextGrammar* grammar, Instruction instruction)
    : grammarRef_(grammar)
    , instruction_(instruction)
    , matchRegExp_(0)
    , endRegExpString_()
{
}


/// The tex text grammar rule
TextGrammarRule::~TextGrammarRule()
{
    qDeleteAll(ruleList_);
    ruleList_.clear();
    delete matchRegExp_;
}


/// Creates a main grammar rule
/// @param grammar the grammar this main rule belongs to
/// @param scopeName the name of the scope
/// @return the main grammar rule
TextGrammarRule* TextGrammarRule::createMainRule(TextGrammar* grammar, const QString& scopeName)
{
    TextGrammarRule* rule = new TextGrammarRule( grammar, MainRule );
    rule->setScopeName( scopeName );
    return rule;
}


/// Creates a grammar rule list
/// @param grammar the grammar this rule list belongs to
/// @return the TextGrammarRule that include the rule list
TextGrammarRule* TextGrammarRule::createRuleList(TextGrammar* grammar)
{
    TextGrammarRule* rule = new TextGrammarRule( grammar, RuleList );
    return rule;
}


/// Creates an include rule
/// @param grammar the grammar this rules belongs to
/// @param includeName the name to include
/// @return an include grammar rule
TextGrammarRule* TextGrammarRule::createIncludeRule(TextGrammar* grammar, const QString& includeName)
{
    TextGrammarRule* rule = new TextGrammarRule( grammar, IncludeCall );
    rule->setIncludeName(includeName);
    return rule;
}


/// Creates an single line regexp
/// @param grammar the grammar this rule belongs to
/// @param scopeName the scopename of this rule
/// @param regExp the regular expression used for this rule
/// @return the created grammar rule
TextGrammarRule* TextGrammarRule::createSingleLineRegExp(TextGrammar* grammar, const QString& scopeName, const QString& regExp)
{
    TextGrammarRule* rule = new TextGrammarRule( grammar, SingleLineRegExp );
    rule->setScopeName( scopeName );
    rule->giveMatchRegExp( TextGrammarRule::createRegExp( regExp ) );
    return rule;
}


/// Creates a multiline regexp
/// @param scopeName the name of the scope
/// @param contentScopeName the scope name of the content
/// @param beginRegExp the start regexp to use
/// @param endRegExp the end regular expression to use
/// @return TextGrammarRule the multiple grammar rule
TextGrammarRule* TextGrammarRule::createMultiLineRegExp( TextGrammar* grammar, const QString& scopeName, const QString& contentScopeName, const QString& beginRegExp, const QString& endRegExp )
{
    TextGrammarRule* rule = new TextGrammarRule( grammar, MultiLineRegExp );
    rule->setScopeName(scopeName);
    rule->setContentScopeName(contentScopeName);
    rule->giveMatchRegExp( TextGrammarRule::createRegExp( beginRegExp ));
    rule->setEndRegExpString( endRegExp );
    return rule;
}


/// returns the child rule at the given index
/// @param idx the index of the grammar rule
TextGrammarRule* TextGrammarRule::rule(int idx) const
{
    Q_ASSERT( 0 <= idx && idx < ruleCount() );
    return ruleList_.at(idx);
}


/// Gives the rulle to the rule-list of the grammar rule
/// @param the rule to give (ownership is transfered to this object)
void TextGrammarRule::giveRule(TextGrammarRule* rule)
{
    ruleList_.append(rule);
}


/// Gives the main regular expression
/// @param regExp the regular expression to give
void TextGrammarRule::giveMatchRegExp(RegExp* regExp)
{
    matchRegExp_ = regExp;
}


/// Sets the ends regular expression(only for multi-line regexp rules
/// @param str the end regular expression
void TextGrammarRule::setEndRegExpString(const QString& str)
{
    endRegExpString_ = str;
}


/// convers the curent object toString (for debugging purposes)
/// @param includePatters should the patterns be included
QString TextGrammarRule::toString(bool includePatterns)
{
    QString r;
    switch( instruction_ ) {
        case MainRule: r.append("MainRule"); break;
        case RuleList: r.append("RuleList"); break;
        case SingleLineRegExp: r.append("SingleLineRegExp"); break;
        case MultiLineRegExp: r.append("MultiLineRegExp"); break;
        case IncludeCall: r.append("IncludeCall");break;
        case Parser: r.append("Parser"); break;
        default: r.append("Unkown");
    }

    r.append(":").append( scopeName_ );
    if( instruction_ == IncludeCall ) {
        r.append(" ");
        r.append(includeName());
    }

    if( includePatterns && matchRegExp_ ) { r.append(", begin: ").append( matchRegExp_->pattern() ); }
    if( includePatterns && !endRegExpString_.isEmpty() ) { r.append(", end: ").append( endRegExpString_ ); }
    r.append( QString(", %1 subrules").arg(ruleCount() ) );
    r.append( QString(", %1 captures").arg(matchCaptures().size()));
    return r;
}


/// parses the given string as a regexp
/// @param regexp the regular expression string to create a regexp from
/// @return the RegExp object
RegExp* TextGrammarRule::createRegExp(const QString& regexp)
{
    RegExp* result = new RegExp( regexp, RegExp::EngineOniguruma ); //, Qt::CaseSensitive, QRegExp::RegExp2 );
    if( !result->isValid() ) {
        qlog_warn() << "Error in regexp: " << result->errorString() << "\n" << regexp;
    }
    return result;
}



//==========================


/// The default texgrammar constructor
/// @param name the name of the textgrammar
/// @param displayName th name to display
TextGrammar::TextGrammar(const QString& name, const QString& displayName)
    : name_(name)
    , displayName_(displayName)
    , mainRule_(0)
{

}


/// The textgrammar destructor
TextGrammar::~TextGrammar()
{
    qDeleteAll( repository_ );
    repository_.clear();
    delete mainRule_;
}


/// Gives the main rule the grammmar
void TextGrammar::giveMainRule(TextGrammarRule* mainRule)
{
    Q_ASSERT(!mainRule_);
    mainRule_ = mainRule;
}


/// This method adds the given grammar rule to the repos
void TextGrammar::giveToRepos(const QString& name, TextGrammarRule* rule)
{
    repository_.insert(name,rule);
}


/// Finds the grammar rule from the respos
/// @param name the name of the rule
/// @param defValue the grammar rule to return if not found
/// @return the found grammar rule (or the defValue if not found)
TextGrammarRule *TextGrammar::findFromRepos(const QString& name, TextGrammarRule* defValue )
{
    return repository_.value(name, defValue );
}


/// Adds a file extension
/// @param ext the extension to add.
void TextGrammar::addFileExtension(const QString& ext)
{
    fileExtensions_.append(ext);
}


//==========================


/// The text grammar manager constructor
TextGrammarManager::TextGrammarManager()
    : defaultGrammarRef_(0)
{

    // always make sure there's a default grammar
    defaultGrammarRef_ = new TextGrammar( "", "Plain Text" );
    defaultGrammarRef_->giveMainRule( TextGrammarRule::createMainRule(defaultGrammarRef_,"text.plain") );
    giveGrammar( defaultGrammarRef_ );
}


/// The detstructor (deletes all grammars)
TextGrammarManager::~TextGrammarManager()
{
    qDeleteAll( grammarMap_ );
    grammarMap_.clear();
}


/// reads all grammar files in the given path
/// @param path the path to read all grammar files from
void TextGrammarManager::readAllGrammarFilesInPath(const QString& path )
{
//    qlog_info() << "readAllGrammarFilesInPath(" << path << ")";
    QDir dir(path);
    QStringList filters("*.tmLanguage");
    TmLanguageParser parser;
    foreach( QFileInfo fileInfo, dir.entryInfoList( filters, QDir::Files, QDir::Name ) ) {
//        qlog_info() << "- parse" << fileInfo.baseName() << ".";
        QFile file( fileInfo.absoluteFilePath() );

        // read the file
        TextGrammar* grammar = 0;
        if( file.open( QIODevice::ReadOnly) ) {
            grammar = parser.readContent( &file );
            file.close();
            if( grammar ) {
                giveGrammar( grammar );
            } else {
                qlog_warn() << QObject::tr("Error reading file %1:%2").arg(fileInfo.absoluteFilePath()).arg(parser.lastErrorMessage());
            }
        } else {
            qlog_warn() << QObject::tr("Error reading file %1:%2").arg(fileInfo.absoluteFilePath()).arg(file.errorString());
        }
    }
}


/// This method returns the given language grammar
TextGrammar* TextGrammarManager::get(const QString &name)
{
    return grammarMap_.value(name,0);
}


/// This method gives a language grammar to the document
/// @param grammar the grammar to give
void TextGrammarManager::giveGrammar(TextGrammar* grammar)
{
    const QString name = grammar->name();
    if( grammarMap_.contains(name)) { delete grammarMap_.take(name); }
    grammarMap_.insert(name,grammar);
}


/// This method returns all grammar names
QList<QString> TextGrammarManager::grammarNames()
{
    return grammarMap_.keys();
}


/// This method returns the values
QList<TextGrammar*> TextGrammarManager::grammars()
{
    return grammarMap_.values();
}


/// this method detects the grammar with a given filename
/// @param fileName the filename to detect the grammar
/// @return the defaultGrammar if no grammar was found
TextGrammar* TextGrammarManager::detectGrammarWithFilename(const QString& fileName)
{
    foreach( TextGrammar* grammar, grammarMap_.values() ) {
        foreach( QString ext, grammar->fileExtensions() ) {
            if( fileName.endsWith( QString(".%1").arg(ext) ) ) return grammar;
        }
    }
    return this->defaultGrammar();
}


} // edbee
