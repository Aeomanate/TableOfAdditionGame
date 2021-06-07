#include <iostream>
#include <iomanip>
#include <fstream>
#include <random>
#include <chrono>
#include <set>
#include <memory>
#include <cstring>
#include <algorithm>
#include <array>
using namespace std::chrono_literals;

int getRandDigit() {
    static std::mt19937 generator(std::chrono::system_clock::now().time_since_epoch().count());
    static std::uniform_int_distribution generator_mask(1, 9);
    static auto rand = [] { return generator_mask(generator); };
    return rand();
}

namespace Chars {
  int getChar() {
      std::cin.clear();
      return std::cin.get();
  }
  int waitEnter() {
      char user_ans;
      
      while(std::cin.peek() != '\n') {
          getChar();
      }
      
      return getChar();
  }
  void clearScreen() {
      auto cls = "printf \"\033c\"";
      system(cls);
  }
  
  template <class... Params>
  void table_params(Params const&&... params) {
      (std::cout << ... << params);
  }
  
  template <class Arg, class... Other>
  void table(int width, Arg&& param, Other&&... other) {
      std::cout << std::setw(width) << param;
      if constexpr (sizeof...(other) != 0) {
          table(std::move(other)...);
      }
  }
  
  template <class String>
  static inline void ltrim(String&& s) {
      s.erase(s.begin(), std::find_if(s.begin(), s.end(),
              [](unsigned char ch) {
                  return !std::isspace(ch);
              }
      ));
  }
  template <class String>
  static inline void rtrim(String&& s) {
      s.erase(
          std::find_if(
              s.rbegin(), s.rend(),
              [](unsigned char ch) {
                  return !std::isspace(ch);
              }
          ).base(),
          s.end()
      );
  }
  template <class String>
  inline auto trim(String&& s) {
      ltrim(s);
      rtrim(s);
      return s;
  }
}
namespace Time {
  template<class Ret, class... Params>
  std::pair<Ret, uint64_t>
  getMcsWorkTime(Ret(* f)(Params...), Params&& ... params) {
      auto const& now = &std::chrono::steady_clock::now;
      using mcs = std::chrono::microseconds;
      
      auto t1 = now();
      Ret ret = f(std::forward(params)...);
      auto t2 = now();
      
      return { ret, std::chrono::duration_cast<mcs>(t2 - t1).count() };
  }
  
  class Timer {
    public:
      template<class TimeDurationType>
      bool isExpire(TimeDurationType duration) {
          if(now() - current > duration) {
              current = now();
              return true;
          }
          return false;
      }
      
      template <class Duration>
      void waitFor(Duration duration) {
          while(!isExpire(duration));
      }
    
    private:
      using clock = std::chrono::steady_clock;
      using ns = std::chrono::nanoseconds;
      static auto constexpr now = &clock::now;
      std::chrono::time_point<clock, ns> current = now();
  };
}
namespace Files {
  template <class GameResult>
  std::multiset<GameResult> readGameData(std::string const& filename) {
      std::ifstream in(filename, std::ios::binary);
      std::multiset<GameResult> readed;
      while(true) {
          GameResult data;
          if(in.read((char*)&data, sizeof(GameResult))) {
              readed.insert(data);
          } else {
              break;
          }
      }
      return readed;
  }
  
  template <class GameResult>
  void writeGameData(std::multiset<GameResult> const& game_results, char const* filename) {
      size_t data_size = sizeof(GameResult);
      std::ofstream out(filename, std::ios::binary);
      auto [i, it] = std::pair{0, game_results.begin()};
      while(i != 10 and it != game_results.end()) {
          out.write((char const*)&*it, sizeof(GameResult));
          ++i, ++it;
      }
  }
}

bool runGame() {
    // Q: A + B = User ans
    // ---------- Ans (Yes\No = right ans)
    int a = getRandDigit();
    int b = getRandDigit();
    std::cout << "Q: " << a << " + " << b << " = ";
    int ans = 0;
    std::cin >> ans;
    
    std::cout << "-----";
    bool is_yes = ans == a + b;
    if(is_yes) {
        std::cout << "----- YES!";
    } else {
        std::cout << " Ans: " << a + b << "\n";
    }
    
    return is_yes;
}

class Game {
  public:
    virtual void run() = 0;
    virtual void top() = 0;
    
    static void prepareToNewRound() {
        Chars::getChar();
        Time::Timer().waitFor(500ms);
        Chars::clearScreen();
    }
};

class EndlessGame: public Game {
  public:
    void run() override {
        while(true) {
            runGame();
            std::cout << "\nContinue? (Enter - yes, 0 - no): ";
            Chars::waitEnter();
            int user_ans = Chars::getChar();
            Chars::clearScreen();
            if(user_ans == '0') {
                return;
            }
        }
    }
    
    void top() override {
        throw std::logic_error("No top for endless game");
    }
};

class GameWithScores: public Game {
  public:
    struct GameResult {
        char nickname[16] = { };
        uint32_t scores = 0;
        
        bool operator<(GameResult const& other) const {
            return scores > other.scores;
        }
        
        void setNickname(std::string const& new_nickname) {
            std::strcpy(nickname, new_nickname.c_str());
        }
    
        static std::string inputNickname() {
            static std::string nickname;
            std::string new_nickname;
            size_t max_nickname_length = sizeof(GameResult::nickname) - 1;
        
            bool reinput_nickname = true;
            while(reinput_nickname) {
                if(nickname.empty()) {
                    std::cout << "Enter nickname: ";
                } else {
                    std::cout << "Nickname [" << nickname << "].\n";
                    std::cout << "Redefine (empty to skip): ";
                }
    
                std::getline(std::cin, new_nickname);
                
                if(!new_nickname.empty()) {
                    nickname = new_nickname;
                }
                if(nickname.size() <= max_nickname_length) {
                    reinput_nickname = false;
                    Chars::clearScreen();
                } else {
                    std::cout << "Too long! (Max 15 symbols)\n";
                    Chars::clearScreen();
                }
            
            }
        
            return nickname;
        }
    };
  
  public:
    GameWithScores()
    : game_results(Files::readGameData<GameResult>(SCORES_FILENAME))
    { }
    
    void run() override {
        GameResult game_result = runGameFor30s(GameResult::inputNickname());
        game_results.insert(game_result);
        Files::writeGameData(game_results, SCORES_FILENAME);
    
        std::cout << game_result.nickname
                  << " score: " << game_result.scores << std::endl;
        Time::Timer().waitFor(1500ms);
        Chars::clearScreen();
    }
    
    void top() override {
        int nick_size = sizeof(GameResult::nickname) - 1;
        
        Chars::table_params(std::left);
        Chars::table(4, "", nick_size + 3, "Nickname", 5, "Score", 0, "\n");
    
        for(auto [i, it] = std::pair{0, game_results.begin()}; i != 10; ++i) {
            Chars::table_params(std::right);
            Chars::table(2, i + 1, 0, ". ");
            Chars::table_params(std::left);
            
            if(it != game_results.end()) {
                Time::Timer().waitFor(50ms);
                Chars::table(nick_size, it->nickname, 3, "", 5, it->scores);
                ++it;
            }
            Chars::table(0, "\n");
        }
        Chars::waitEnter();
        Chars::clearScreen();
    }
  
  private:
    static GameResult runGameFor30s(std::string const& nickname) {
        GameResult game_result;
        game_result.setNickname(game_result.nickname);
    
        uint64_t game_delay_mcs = 3 * 1000 * 1000;
        uint64_t cur_game_duration_mcs = 0;
        while(cur_game_duration_mcs <= game_delay_mcs) {
            auto [is_yes, mcs] = Time::getMcsWorkTime(&runGame);
            game_result.scores += is_yes;
            cur_game_duration_mcs += mcs;
            
            prepareToNewRound();
        }
        return game_result;
    }
    
  private:
    static char const constexpr* SCORES_FILENAME = "game_scores_30s.bin";
    std::multiset<GameResult> game_results;
};

class GameWith3Errors: public Game {
  public:
  struct GameResult: public GameWithScores::GameResult {
      uint64_t game_duration_mcs = 0;
      bool operator<(GameResult const& other) const {
          return scores > other.scores or
                 game_duration_mcs < other.game_duration_mcs;
      }
      [[nodiscard]] std::string getTimeSec() const {
          uint64_t game_dur_ms = game_duration_mcs / 1000;
          uint64_t game_dur_s = game_dur_ms / 1000;
          uint64_t game_dur_s_rest = game_dur_ms % 1000;
          std::string game_duration;
          game_duration += std::to_string(game_dur_s / 60) + "m ";
          game_duration += std::to_string(game_dur_s) + ".";
          game_duration += std::to_string(game_dur_s_rest) + "s";
          return game_duration;
      }
  };
  
  public:
    GameWith3Errors()
    : game_results(Files::readGameData<GameResult>(SCORES_FILENAME))
    { }
    
    
    void run() override {
        GameResult game_result = runGameWith3Errors(GameResult::inputNickname());
        game_results.insert(game_result);
        Files::writeGameData(game_results, SCORES_FILENAME);
    
        std::cout << game_result.nickname
                  << " score: " << game_result.scores << ", "
                  << "time: " << game_result.getTimeSec() << std::endl;
        Time::Timer().waitFor(1500ms);
        Chars::clearScreen();
    }
    
    void top() override {
        int nick_size = sizeof(GameResult::nickname) - 1;
        static int max_game_duration_size = static_cast<int>(strlen(("100m 59.999s")));
    
        Chars::table_params(std::left);
        Chars::table(4, "", nick_size + 3, "Nickname", 5 + 3, "Score");
        Chars::table(max_game_duration_size, "Time", 0, "\n");
    
        for(auto [i, it] = std::pair{0, game_results.begin()}; i != 10; ++i) {
            Chars::table_params(std::right);
            Chars::table(2, i + 1, 0, ". ");
            Chars::table_params(std::left);
        
            if(it != game_results.end()) {
                Time::Timer().waitFor(50ms);
                
                Chars::table(nick_size + 3, it->nickname, 5 + 3, it->scores);
                Chars::table(max_game_duration_size, it->getTimeSec());
                ++it;
            }
            Chars::table(0, "\n");
        }
        Chars::waitEnter();
        Chars::clearScreen();
    }
  
  private:
    static GameResult runGameWith3Errors(std::string const& nickname) {
        size_t errors = 3;
        size_t cur_errors = 0;
        
        GameResult game_result;
        game_result.setNickname(game_result.nickname);
        
        while(cur_errors < errors) {
            auto [is_yes, mcs] = Time::getMcsWorkTime(&runGame);
            game_result.scores += is_yes;
            cur_errors += !is_yes;
            game_result.game_duration_mcs += mcs;
    
            prepareToNewRound();
        }
        return game_result;
    }
  
  private:
    static char const constexpr* SCORES_FILENAME = "game_scores_3errors.bin";
    std::multiset<GameResult> game_results;
};

void menu() {
    bool is_menu_work = true;
    
    std::array<std::unique_ptr<Game>, 3> gameVariants;
    gameVariants[0] = std::make_unique<EndlessGame>();
    gameVariants[1] = std::make_unique<GameWithScores>();
    gameVariants[2] = std::make_unique<GameWith3Errors>();
    
    while(is_menu_work) {
        std::cout << "1. Run without scores\n";
        std::cout << "2. Run with scores for 30 sec\n";
        std::cout << "3. Run with scores for 3 errors\n";
        std::cout << "4. Show top \"30s\" scores\n";
        std::cout << "5. Show top \"3 errors\" scores\n";
        std::cout << "6. Exit\n";
        std::cout << "Your choice: ";
        int ans = 0;
        std::cin >> ans;
        Chars::getChar();
        
        if(1 <= ans and ans <= 3) {
            Chars::clearScreen();
            gameVariants[ans-1]->run();
        } else if(4 <= ans and ans <= 5) {
            Chars::clearScreen();
            gameVariants[1 + ans-4]->top();
        } else if(ans == 6) {
            is_menu_work = false;
        } else {
            std::cout << "Wrong choice, try again\n";
            Chars::waitEnter();
            Chars::clearScreen();
        }
    
    }
}

int main() {
    menu();
    return 0;
}
