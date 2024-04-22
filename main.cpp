#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

using namespace std;

//-------------------------------------------------------------------------------------------------
// HELPER METHODS
//-------------------------------------------------------------------------------------------------

vector<string> read_file(string file_path)
{
  vector<string> lines{};
  fstream file_handler(file_path.c_str());

  if (file_handler.fail())
  {
    cout << "Error: can't read from file: " << file_path << endl;
    return lines;
  }

  string line;
  while (getline(file_handler, line))
  {
    lines.push_back(line);
  }

  file_handler.close();
  return lines;
}

void write_to_file(string file_path, vector<string> lines, bool append = true)
{
  auto status = append
    ? ios::in | ios::out | ios::app
    : ios::in | ios::out | ios::trunc;

  fstream file_handler(file_path.c_str(), status);
  if (file_handler.fail())
  {
    cout << "Error: can't write to file: " << file_path << endl;
    return;
  }

  for (const string &line : lines)
  {
    file_handler << line << endl;
  }

  file_handler.close();
}

vector<string> split_string(string text, char delimiter = ',')
{
  vector<string> sub_strings{};
  stringstream sst(text);

  string segment{};
  while (getline(sst, segment, delimiter))
  {
    sub_strings.push_back(segment);
  }

  return sub_strings;
}

int build_menu(const vector<string>& options)
{
  int user_choice {};
  int counter {};

  cout << "Menu:" << endl;
  for (const string& option : options)
  {
    cout << "\t " << ++counter << ": " << option << endl;
  }

  cout << endl;
  cout << "Enter number in range 1 - " << counter << ": ";
  cin >> user_choice;
  cout << endl;
  
  return user_choice; 
}

//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
// ENTITIES
//-------------------------------------------------------------------------------------------------

enum menu_type
{
  SIGN,
  OPTIONS
};

struct user
{
  int id, allow_anonymous_questions;
  string username, password, name, email;
  vector<int> from_question_ids {};

  // map contains a parent question and the corresponding thread if found.
  vector<int> to_user_questions_ids;
  map<int, vector<int>> to_user_questions_to_threan_ids;

  user()
  {
    id = allow_anonymous_questions = -1;
  }

  string to_csv_line() const
  {
    stringstream sst{};
    sst << id << "," << username << "," << password << ","
        << name << "," << email << "," << allow_anonymous_questions;

    return sst.str();
  }

  void print() const
  {
    cout << "ID: " << id << "\t\t" << "Name: " << name << endl;
  }
};

struct question
{
  int id, parent_question_id, from_user_id, to_user_id;
  string question_text;
  string answer_text;
  bool is_anonymous;

  const string not_answered_msg = "NOT answered YET!";

  question()
  {
    id = from_user_id = to_user_id = -1;
  }

  void print(bool from_me_question = false) const
  {
    cout << "Question Id (" << id << ") from user id (" << from_user_id << ")" << "\t Question: " << question_text << endl;
    
    if (answer_text != "")
      cout << "\t Answer: " << answer_text << endl;
    else if (from_me_question)
      cout << "\t " << not_answered_msg << endl;
    
    cout << endl;
  }

  void print_as_thread() const
  {
    cout << "\t Thread: Question Id (" << id << ") from user id (" << from_user_id << ")" << "\t Question: " << question_text << endl;
    if (answer_text != "")
      cout << "\t Thread: \t Answer: " << answer_text << endl;
    cout << endl;
  }

  string to_csv_line() const
  {
    stringstream sst{};

    sst << id << "," << parent_question_id << "," << from_user_id << ","
        << to_user_id << "," << is_anonymous << "," << question_text << ","
        << answer_text << ",";

    return sst.str();
  }
};

//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
// GLOBAL VARIABLES
//-------------------------------------------------------------------------------------------------

string invalid_input_msg = "Invalid Input .. Try Again!\n\n";

map<menu_type, vector<string>> menu = { 
  { menu_type::SIGN, { "Sign In", "Sign Up" } },  
  { menu_type::OPTIONS, { 
    "Print Questions To Me", "Print Questions From Me",
    "Answer Question", "Delete Question",
    "Ask Question", "List System Users",
    "List Feed", "Logout" } 
  }  
};

//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
// SERVICES
//-------------------------------------------------------------------------------------------------

struct user_service
{
  const string users_db_path = "users.txt";
  const string user_not_registered_msg = "The Entered User Is Not Registered!\n";

  int last_recorded_user_id = -1;
  vector<user> system_users{};
  user current_user{};

  user parse_from_line(const string &line)
  {
    vector<string> fields = split_string(line);

    user _user{};
    _user.id = stoi(fields[0]);
    _user.username = fields[1];
    _user.password = fields[2];
    _user.name = fields[3];
    _user.email = fields[4];
    _user.allow_anonymous_questions = stoi(fields[5]);

    return _user;
  }

  void load_users()
  {
    vector<string> users_rows = read_file(users_db_path);
    for (const string& user_row : users_rows)
    {
      user loaded_user = parse_from_line(user_row);
      system_users.push_back(loaded_user);

      last_recorded_user_id = loaded_user.id > last_recorded_user_id
        ? loaded_user.id 
        : last_recorded_user_id;
    }
  }

  bool user_exists(const string &username, const string &password, user &o_user)
  {
    for (const auto u : system_users)
    {
      if (u.username == username && u.password == password)
      {
        o_user = u;
        return true;
      }
    }

    return false;
  }

  bool get_user_by_name(const string &username, user &o_user)
  {
    for (const user& u : system_users)
    {
      if (u.username == username)
      {
        o_user = u;
        return true;
      }
    }

    return false;
  }

  bool get_user_by_id(int user_id, user& o_user)
  {
    auto it = find_if(
      system_users.begin(), 
      system_users.end(), 
      [&user_id] (const user& _user) { return _user.id == user_id; });

    if (it != system_users.end()) 
    {
      o_user = *it;
      return true;
    }

    return false;
  }

  void add_new_user()
  {
    while (true) {
			cout << "Enter user name (No spaces): ";
			cin >> current_user.username;

			if (get_user_by_name(current_user.username, current_user))
				cout << "User Already Exists! Enter nother name Try again\n";
			else
				break;
		}
		cout << "Enter password: ";
		cin >> current_user.password;

		cout << "Enter name: ";
		cin >> current_user.name;

		cout << "Enter email: ";
		cin >> current_user.email;

		cout << "Allow anonymous questions? (0 or 1): ";
		cin >> current_user.allow_anonymous_questions;

		current_user.id = ++last_recorded_user_id;

    update_databse(current_user);
  }

  bool authenticate()
  {
    while (true)
    {
      int user_choice = build_menu(menu[menu_type::SIGN]);
      if (1 == user_choice)
      {
        string username;
        string password;
        cout << "Eneter username & password: ";
        cin >> username >> password;

        return user_exists(username, password, current_user);
      }
      else if (2 == user_choice)
      {
        add_new_user();
        return true;
      }
      else
      {
        cout << invalid_input_msg;
      }
    }
  }

  void update_databse(user new_user)
  {
    write_to_file(users_db_path, {new_user.to_csv_line()});
  }
};

struct question_service
{
  const string questions_db_path = "questions.txt";

  int last_recorded_question_id = -1;
  map<int, question> ids_to_questions_map;

  void load_questions()
  {
    vector<string> question_rows = read_file(questions_db_path);
    for (const string& question_row : question_rows)
    {
      question loaded_question = parse_from_line(question_row);
      ids_to_questions_map.insert({ loaded_question.id, loaded_question });

      last_recorded_question_id = loaded_question.id > last_recorded_question_id
        ? loaded_question.id
        : last_recorded_question_id;
    }
  }

  question parse_from_line(const string &line)
  {
    vector<string> fields = split_string(line);

    question question{};
    question.id = stoi(fields[0]);
    question.parent_question_id = stoi(fields[1]);
    question.from_user_id = stoi(fields[2]);
    question.to_user_id = stoi(fields[3]);
    question.is_anonymous = stoi(fields[4]);
    question.question_text = fields[5];
    question.answer_text = fields[6];

    return question;
  }

  void laod_user_questions(user& user)
  {
    vector<question> to_user_questions;

    for (const auto& _question_map : ids_to_questions_map)
    {
      if (_question_map.second.from_user_id == user.id)
      {
        user.from_question_ids.push_back(_question_map.first);
      }

      if (_question_map.second.to_user_id == user.id)
      {
        to_user_questions.push_back(_question_map.second);
        user.to_user_questions_ids.push_back(_question_map.first);

        if (_question_map.second.parent_question_id == -1)
          user.to_user_questions_to_threan_ids.insert( { _question_map.first, {} } );
      }
    }

    map_question_to_threads(user, to_user_questions);
  }

  void map_question_to_threads(user& user, vector<question>& to_user_question)
  {
    for (const question& _question : to_user_question)
    {
      if (_question.parent_question_id != -1)
      {
        user.to_user_questions_to_threan_ids[_question.parent_question_id].push_back(_question.id);
      }
    }
  }

  vector<question> get_questions_by_ids(vector<int> ids) const
  {
    vector <question> returned_questions;
    for (const auto& _question_map : ids_to_questions_map)
    {
      for (const int& id : ids)
      {
        if (_question_map.first == id)
          returned_questions.push_back(_question_map.second);
      }
    }
    return returned_questions;
  }

  bool ask_for_question_id(int& question_id)
  {
    question_id = -1;

    cout << "Enter question id or -1 to cancel: ";
    cin >> question_id;
    if (question_id == -1) return false;
    
    if (!ids_to_questions_map.count(question_id))
    {
      cout << "Question id is not valid!" << endl;  
      return false;
    }

    return true;
  }

  question build_question(
    int parent_question_id,
    int from_user_id,
    int to_user_id,
    int is_anonymous,
    string question_text)
  {
    question new_question;
    new_question.id = ++last_recorded_question_id;
    new_question.parent_question_id = parent_question_id;
    new_question.from_user_id = from_user_id;
    new_question.to_user_id = to_user_id;
    new_question.is_anonymous = is_anonymous;
    new_question.question_text = question_text;
    return new_question;
  }

  void answer_question()
  { 
    int question_id;
    if (!ask_for_question_id(question_id)) return;
    
    question& _question = ids_to_questions_map[question_id];
    cout << endl;
    _question.print();

    if (_question.answer_text != "")
      cout << endl << "Warning: already answered. Answer will be updated" << endl;

    string answer;
    cout << "Enter answer: ";
    getline(cin, answer);
    getline(cin, answer);

    if (answer != "")
      _question.answer_text = answer;

    update_database();
  }

  void delete_question(user& _user)
  { 
    int question_id;
    if (!ask_for_question_id(question_id)) return;
    question& _question = ids_to_questions_map[question_id];

    // the question is a thread question:
    if (_question.parent_question_id == -1)
    {
      auto it = _user.to_user_questions_to_threan_ids.find(_question.id);
      if (it != _user.to_user_questions_to_threan_ids.end())
      {
        vector<int> thread_ids = it->second;
        for (const auto& id : thread_ids)
          ids_to_questions_map.erase(id);
      }
    }

    _user.to_user_questions_to_threan_ids.erase(_question.id);
    ids_to_questions_map.erase(_question.id);
    update_database();
  }

  void ask_question(user& from_user, user& to_user)
  {
    if (!to_user.allow_anonymous_questions)
      cout << "Note: Anonymous questions are not allowed for this user!" << endl;
    
    int question_id;
    cout << "For thread question: Enter question id or -1 for new question: ";
    cin >> question_id;

    if (question_id != -1 && !ids_to_questions_map.count(question_id))
      cout << "No question has the inserted question id!" << endl;
    int parent_question_id = question_id == -1 ? -1 : question_id;

    string question_text;
    cout << "Enter question text: ";
    getline(cin, question_text);
    getline(cin, question_text);

    question new_question = build_question(parent_question_id, from_user.id, to_user.id, to_user.allow_anonymous_questions, question_text);
    ids_to_questions_map.insert( { new_question.id, new_question } );
    from_user.from_question_ids.push_back(new_question.id);
    
    update_database();
  }

  void list_feed()
  {
    for (auto& map_item : ids_to_questions_map)
    {
      question& _question = map_item.second;
      if (_question.answer_text == "")
        continue;

      _question.print();
    }
  }

  void update_database()
  {
    vector<string> lines;
    for (const auto& pair : ids_to_questions_map)
      lines.push_back(pair.second.to_csv_line());

    write_to_file(questions_db_path, lines, false);
  }
};

//-------------------------------------------------------------------------------------------------

struct ask_me_system
{
private:
  user_service user_service;
  question_service question_service;

  void load_db()
  {
    user_service.load_users();
    question_service.load_questions();
  }

public:
  void run()
  {
    load_db();

    if (user_service.authenticate())
    {
      question_service.laod_user_questions(user_service.current_user);
      while (true)
      {
        int user_choice = build_menu(menu[menu_type::OPTIONS]);
        if (1 == user_choice)
        {
          for (const auto& mapped_item : user_service.current_user.to_user_questions_to_threan_ids)
          {
            question& parent_question = question_service.ids_to_questions_map[mapped_item.first];
            parent_question.print();

            if ((int)mapped_item.second.size() == 0) continue;
            vector<question> threads = question_service.get_questions_by_ids(mapped_item.second);
            for (const question& thread : threads)
              thread.print_as_thread();
          }
        }
        else if (2 == user_choice)
        {
          vector<question> from_user_questions = question_service
            .get_questions_by_ids(user_service.current_user.from_question_ids);

          if ((int)from_user_questions.size() == 0)
            cout << "NO QUESTION FOUND!" << endl;
          else 
          { 
            for (const question& question : from_user_questions)
              question.print(true);

            cout << endl;
          }
        }
        else if (3 == user_choice)
        {
          question_service.answer_question();
        }
        else if (4 == user_choice)
        {
          question_service.delete_question(user_service.current_user);
        }
        else if (5 == user_choice)
        {
          int to_user_id;
          cout << "Enter user id or -1 to cancel: ";
          cin >> to_user_id;
          if (to_user_id == -1) continue;

          user to_user;
          if (!user_service.get_user_by_id(to_user_id, to_user)) return;
          question_service.ask_question(user_service.current_user, to_user);
        }
        else if (6 == user_choice)
        {
          for (const user& _user : user_service.system_users)
            _user.print();
          cout << endl;
        }
        else if (7 == user_choice)
        {
          question_service.list_feed();
        }
        else if (8 == user_choice)
        {
          break;
        }
        else
        {
          cout << invalid_input_msg;
        }
      }
    }
  }
};

int main()
{
  ask_me_system ask_system;
  ask_system.run();

  return 0;
}

/*
101,-1,11,13,0,Should I learn C++ first or Java,I think C# is a better choice!,
203,101,11,13,0,Why do you think so!,Just Google. There is an answer on Quora.,
205,101,45,13,0,What about python?,,
211,-1,13,11,1,It was nice to chat to you,For my pleasure Dr Mostafa,
212,-1,13,45,0,Please search archive before asking,,
300,101,11,13,1,Is it ok to learn Java for OOP?,Good choice,
301,-1,11,13,0,Free to meet?,,
302,101,11,13,1,Why so late in reply?,,
*/