/*INCLUDES */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#include <string.h>

/* DEFINES */
#define MAX_TEAMS 24
#define MAX_WEEKS 20
#define MAX_GAMES_PER_WEEK 24
#define MAX_GAMES MAX_WEEKS * MAX_GAMES_PER_WEEK

#define MAX_ATTEMPTS 1000
#define MAX_SCHEDULE_ATTEMPTS 100

/* STRUCTURES */
typedef struct _gamestruct{
    uint8_t time;
    uint8_t court;
    uint8_t teamA;
    uint8_t teamB;
} GAMESTRUCT, *PGAMESTRUCT;

typedef struct _teamstruct{
    uint8_t opp[MAX_TEAMS];
    uint8_t game_counter;
} TEAMSTRUCT, *PTEAMSTRUCT;

typedef struct _schedulestruct{
    uint8_t week;
    uint16_t games[MAX_GAMES_PER_WEEK];
} SCHEDULESTRUCT, *PSCHEDULESTRUCT;

/* FUNCTIONS */
//Create Functions
uint8_t create_week(FILE* fp, uint8_t week);
uint16_t create_game(uint8_t teamA, uint8_t teamB);
void refill_games(void);
uint16_t fill_games(uint8_t num_teams, uint16_t *array);
uint16_t select_game(uint8_t *teamA, uint8_t *teamB, uint8_t *index);
void add_used_games(uint8_t week, uint16_t num_games);
void save_game(uint8_t index, uint8_t teamA, uint8_t teamB);
//Check Functions
//Update Functions
void update_schedule(uint8_t teamA, uint8_t teamB, uint8_t game_num, uint8_t week);
//Calc Functions
uint8_t calc_num_games_per_week(void);
uint8_t calc_num_sets(void);
uint8_t calc_num_doubleheaders(void);
//Clear Functions
void clear_game_counter(void);
void remove_multiple_games(void);
void remove_game(uint8_t teamA, uint8_t teamB);
//Utility Functions
uint16_t rand_interval(uint16_t min, uint16_t max);
void usage(void);

/* VARIABLES */
uint8_t g_numTeams;
uint8_t g_numWeeks;
uint8_t g_numGames;
uint16_t g_total_index;
uint16_t g_games_used = 0;

//Calculated without double header
uint8_t g_numGamesperWeek;

//Add additional sets of all possiblities when
//the number of weeks is greater than the number
//of teams - 1. You have to play a team more than once
uint8_t g_numSets;

//Number of games needed to be added,
//so everyone plays the same amount of games
uint8_t g_doubleHeaders;
uint8_t g_additionalGames;


SCHEDULESTRUCT schedule[MAX_WEEKS];
TEAMSTRUCT team[MAX_TEAMS];

uint16_t all_games[MAX_GAMES];
uint16_t used_games[MAX_GAMES];
uint16_t saved_games[MAX_GAMES];
uint8_t team_list[30] = {0};
uint32_t g_count;
uint16_t g_team_index = 0;

int main (int argc, char *argv[])
{
    uint8_t week = 1;
    FILE* fp;

    //ARGV[1] TEAMS
    //ARGV[2] WEEKS
    g_numTeams = atoi(argv[1]);
    g_numWeeks = atoi(argv[2]);
    g_doubleHeaders = calc_num_doubleheaders();
    g_numSets = calc_num_sets();
    g_numGamesperWeek = calc_num_games_per_week();

    //How many of complete sets of possiblities are needed?
    refill_games();

    fp = fopen("schedule.xls", "w");

    while(week <= g_numWeeks)
    {
        if(create_week(fp, week))
        {
            //Week failed
            //Refill All games
            refill_games();
            //Remove everything used so far
            remove_multiple_games();
            //Clear Game Counter
            clear_game_counter();
        }
        else
        {
            clear_game_counter();
            printf("\r                                                       \r");
            printf("Week %i Created", week);
            week++;
        }
        g_count = 0;
    }
    fclose(fp);
    return 0;
}
uint8_t create_week(FILE* fp, uint8_t week)
{
    uint8_t teamA, teamB;
    uint8_t index;
    uint16_t game = 0;

    while(game < g_numGamesperWeek)
    {
        //Random Game
        if(select_game(&teamA, &teamB, &index))
        {
            //Unable to Select a Game
            return 1;
        }
        else
        {
            //Checks
            team[teamA - 1].opp[teamB - 1]++;
            team[teamB - 1].opp[teamA - 1]++;
            team[teamA - 1].game_counter++;
            team[teamB - 1].game_counter++;

            //Mark games used
            remove_game(teamA, teamB);
            save_game(game, teamA, teamB);

            //Update Schedule
            update_schedule(teamA, teamB, game, week);
            game++;
        }
    }

    add_used_games(week, game);

    return 0;
}
uint16_t create_game(uint8_t teamA, uint8_t teamB)
{
    uint16_t game = 0;

    game = teamB;
    game |= teamA << 8;

    return game;
}

void refill_games(void)
{
    uint8_t i = 0;
    uint16_t index = 0;

    while(g_numSets > i)
    {
        index = fill_games(g_numTeams, &all_games[index]);
        i++;
        index = index * i;
        g_total_index = index - 1;
    }
}

uint16_t fill_games(uint8_t num_teams, uint16_t *array)
{
    uint8_t teamA,teamB;
    uint8_t i,j,k;
    uint8_t flag = 0;
    uint16_t counter = 0;

    //Fill Teams
    for(i=0; i < num_teams; i++)
    {
        teamA = i+1;
        teamB = 1;

        for(j=0; j < num_teams; j++)
        {
            for(k=0; k < counter; k++)
            {
                if(create_game(teamA, teamB) == array[k])
                {
                    flag = 1;
                }
                if(create_game(teamB, teamA) == array[k])
                {
                    flag = 1;
                }
            }
            if(teamA == teamB)
            {
                flag = 1;
            }
            if(!flag)
            {
                array[counter]= create_game(teamA, teamB);
                counter++;
            }
            teamB++;
            flag = 0;
        }
    }
    return counter;
}

void add_used_games(uint8_t week, uint16_t num_games)
{
    uint16_t i;

    for(i=0; i < num_games; i++)
    {
        used_games[g_games_used] = saved_games[i];
        saved_games[i] = 0;
        ++g_games_used;
    }
}

void save_game(uint8_t index, uint8_t teamA, uint8_t teamB)
{
    uint16_t game;

    game = teamB;
    game = game | (teamA << 8);

    saved_games[index] = game;
}

uint16_t select_game(uint8_t *teamA, uint8_t *teamB, uint8_t *index)
{
    uint8_t flag;
    uint32_t counter = 0;
    uint16_t game;

    while(1)
    {
        //Got into a bad spot
        if(counter > MAX_ATTEMPTS)
        {
            counter = 0;
            return 1;
        }
        counter++;

        flag = 0;
        *index = rand_interval(0, g_total_index);
        game = all_games[*index];
        *teamA = game >> 8;
        *teamB = game & 0x00FF;

        //Has either team played this week?
        if((team[*teamA - 1].game_counter >= 1) || (team[*teamB - 1].game_counter >= 1))
        {
            flag = 1;
        }

        //Passed First Check, Game is good
        if(!flag)
        {
            break;
        }
    }
    return 0;
}
void update_schedule(uint8_t teamA, uint8_t teamB, uint8_t game_num, uint8_t week)
{
    uint16_t game;

    game = teamB;
    game = game | (teamA << 8);

    schedule[week - 1].week = week;
    schedule[week - 1].games[game_num] = game;
}
uint8_t calc_num_games_per_week(void)
{
    //If odd return as many games without a double header
    if(g_numTeams % 2)
    {
        return ((g_numTeams - 1) >> 1);
    }
    //If even return half the teams
    else
    {
        return (g_numTeams >> 1);
    }
}
uint8_t calc_num_sets(void)
{
    uint8_t set = 1;

    while(1)
    {
        if(g_numWeeks > (g_numTeams*set))
        {
            set++;
        }
        else
        {
            return set;
        }
    }
}

uint8_t calc_num_doubleheaders(void)
{
    uint8_t tmp;

    //With 1 Double Header per week
    //Check for Odd Teams
    if(g_numTeams % 2)
    {
        g_numGames = ((g_numTeams + 1) >> 1) * g_numWeeks;
    }
    //If even no double headers needed
    else
    {
         g_numGames = (g_numTeams >> 1) * g_numWeeks;
         return 0;
    }
    while(1)
    {
        tmp = ((g_numGames + g_additionalGames) % g_numTeams);

        if(!tmp)
            break;
        g_additionalGames += 2;
    }

    return (g_numWeeks + g_additionalGames);
}
void clear_game_counter(void)
{
    uint8_t i;

    for(i=0; i < g_numTeams; i++)
    {
        team[i].game_counter = 0;
    }
}

void remove_multiple_games(void)
{
    uint8_t teamA,teamB;
    uint8_t i;

    for(i=0; i < g_games_used; i++)
    {
        teamA = used_games[i] >> 8;
        teamB = used_games[i] & 0x00FF;
        remove_game(teamA, teamB);
    }
}

void remove_game(uint8_t teamA, uint8_t teamB)
{
    uint8_t i;
    uint8_t index;
    uint16_t team;

    //Smaller number always first
    team = teamB;
    team = team | (teamA << 8);

    for(i=0; i < sizeof(all_games); i++)
    {
        if(all_games[i] == team)
        {
            index = i;
            all_games[i] = 0;
            break;
        }
    }

    for(i=index; i < g_total_index; i++)
    {
        all_games[i] = all_games[i+1];
    }

    g_total_index--;
}

uint16_t rand_interval(uint16_t min, uint16_t max)
{
    int16_t r;
    uint16_t range = 1 + max - min;
    uint16_t buckets = RAND_MAX / range;
    uint16_t limit = buckets * range;

    /* Create equal size buckets all in a row, then fire randomly towards
     * the buckets until you land in one of them. All buckets are equally
     * likely. If you land off the end of the line of buckets, try again. */
    do
    {
        r = rand();
    } while (r >= limit);

    return min + (r / buckets);
}
void usage(void)
{
    printf("Hot Shots Schedule Generator v0.2\r\n");
    printf("by Brendan Merna\r\n");
    printf("- [# Teams]\r\n");
    printf("- [# Weeks]\r\n");
    printf("- [# Games Per Week]\r\n");
    printf("- [# Start Hour]\r\n");
    printf("- [# Start Min]\r\n");
}
