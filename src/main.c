#include <stdio.h>
#include <SDL.h>
#include "constants.h"

/*
 * #############################################
 *  TYPE DEFS
 * #############################################
 */
struct game_button_state
{
	int half_transition_count; // transition as in multiple taps of a button for dash actions
	int ended_down;
};

struct game_controller_input
{
	int is_connected;
	int is_analogue; // e.g - true for gamepads, false for keyboards

	// Stick averages used to determine if there
	// was a trigger on our movement buttons
	float stick_average_x;
	float stick_average_y;

	union
	{
		struct game_button_state buttons[2]; // array size same as num struct members below (minus terminator)

		struct {
			struct game_button_state move_up;
			struct game_button_state move_down;

			// NOTE: All buttons must be added above this line 
			// (allows to Assert the Buttons array size is equal to num members above terminator)
			struct game_button_state terminator;
		};
	};
};

struct game_input 
{
	struct game_button_state mouse_buttons[5];
	int mouse_x, mouse_y, mouse_z;

	float dt_for_frame;

	struct game_controller_input controllers[GAME_CONTROLLERS_MAX]; // includes keyboard "controllers"
};

struct game_screen
{
	Uint8 index;
	Uint8 should_run_game;
};

struct ball
{
	float width;
	float height;
	float x;
	float y;
	int dx; // movement vector
	int dy; // movement vector
};

struct paddle
{
	float width;
	float height;
	float x;
	float y;
	int dx; // movement vector
	int dy; // movement vector
	int controller_index;
};

struct score 
{
	int points;
};

struct player
{
	struct score score;
};

struct round
{
	int round_num;
	int elapsed_ms;
	struct player players[PADDLES_NUM_MAX];
};

/*
 * #############################################
 *  GLOBALS
 * #############################################
 */

// Rendering
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

static SDL_Surface* screen_surface;
static SDL_Surface* title_screen;
static SDL_Surface* game_screen_num_map;
static SDL_Surface* game_over_screen;

static SDL_Texture* screen_texture;

// Input
struct game_input input[2]; // 0 = new input, 1 = old input (last frame)
struct game_input* new_input = &input[0];
struct game_input* old_input = &input[1];

// Screen
struct game_screen current_screen;

// Scoreboard
struct round current_round;

// Game Objects
struct paddle paddles[PADDLES_NUM_MAX];
struct ball ball;

// Misc
int is_game_running = FALSE;
int last_frame_time_ms = 0;

/// <summary>
///		Initializes our window and renderer
/// </summary>
int initialize_window()
{
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) 
	{
		fprintf(stderr, "Error initializing SDL.\n");
		return FALSE;
	}

	window = SDL_CreateWindow(
		NULL,
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		WINDOW_WIDTH,
		WINDOW_HEIGHT,
		SDL_WINDOW_BORDERLESS
	);

	if (!window)
	{
		fprintf(stderr, "Error creating SDL Window.\n");
		return FALSE;
	}

	renderer = SDL_CreateRenderer(window, -1, 0);

	if (!renderer)
	{
		fprintf(stderr, "Error creating SDL Renderer.\n");
		return FALSE;
	}

	return TRUE;
}

/// <summary>
///		Destroys our window and renderer then terminates the programme
/// </summary>
void destroy_window()
{
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}

/// <summary>
///		Initializes the screen textures we'll render from
/// </summary>
int init_screen_textures()
{
	screen_texture = SDL_CreateTextureFromSurface(renderer, screen_surface);

	// The colour key to mask out on our bitmaps
	Uint32 colour_key = SDL_MapRGB(title_screen->format, 255, 0, 255);
	SDL_SetColorKey(title_screen, SDL_TRUE, colour_key);
	SDL_SetColorKey(game_screen_num_map, SDL_TRUE, colour_key);
	SDL_SetColorKey(game_over_screen, SDL_TRUE, colour_key);

	if (!screen_texture)
	{
		printf("Could not create screen_texture from screen_surface. SDL Err: %s\n", SDL_GetError());
		return 1;
	}
}

/// <summary>
///		Initializes the screen bitmap surfaces for sprites like score and screen imagery
/// </summary>
int init_screen_surfaces()
{
	// Screen surface we'll draw to and then blit with
	screen_surface = SDL_CreateRGBSurfaceWithFormat(0, WINDOW_WIDTH, WINDOW_HEIGHT, 32, SDL_PIXELFORMAT_RGBA32);

	// Title
	title_screen = SDL_LoadBMP("assets/title_screen.bmp");

	if (!title_screen)
	{
		printf("Could not load the title_screen.bmp surface. SDL Err: %s\n", SDL_GetError());
		return 1;
	}

	// Number map
	game_screen_num_map = SDL_LoadBMP("assets/num_sprite_map.bmp");

	if (!game_screen_num_map)
	{
		printf("Could not load the num_sprite_map.bmp surface. SDL Err: %s\n", SDL_GetError());
		return 1;
	}

	// Game over
	game_over_screen = SDL_LoadBMP("assets/game_over_screen.bmp");

	if (!game_over_screen)
	{
		printf("Could not load the game_over_screen.bmp surface. SDL Err: %s\n", SDL_GetError());
		return 1;
	}
}

void init_scoreboard()
{
	struct score default_score;
	default_score.points = SCORE_MIN;

	current_round.elapsed_ms = 0;
	current_round.players[0].score = current_round.players[1].score = default_score;
	current_round.round_num = 0;
}

/// <summary>
///		Initializes the ball dimensions
/// </summary>
void init_ball_dimensions()
{
	ball.width = BALL_SIZE;
	ball.height = BALL_SIZE;
}

/// <summary>
///		Initializes the ball dimensions
/// </summary>
void init_ball_positions()
{
	ball.x = BALL_X_DEFAULT;
	ball.y = BALL_Y_DEFAULT;
	ball.dx = BALL_DX_DEFAULT;
	ball.dy = BALL_DY_DEFAULT;
}

/// <summary>
///		Initializes the paddle dimensions
/// </summary>
void init_paddles_misc()
{
	paddles[0].controller_index = 0;
	paddles[1].controller_index = 1;
}

/// <summary>
///		Initializes the paddle dimensions
/// </summary>
void init_paddles_dimensions()
{
	paddles[0].width = PADDLE_WIDTH;
	paddles[1].width = PADDLE_WIDTH;
	paddles[0].height = PADDLE_HEIGHT;
	paddles[1].height = PADDLE_HEIGHT;
}

/// <summary>
///		Initializes the paddle positions
/// </summary>
void init_paddles_positions()
{
	paddles[0].y = paddles[1].y = (WINDOW_HEIGHT / 2) - (PADDLE_HEIGHT / 2);
	paddles[0].x = PADDLES_X_OFFSET;
	paddles[1].x = WINDOW_WIDTH - PADDLE_WIDTH - PADDLES_X_OFFSET;
}

/// <summary>
///		Initializes the game
/// </summary>
void init()
{
	init_screen_surfaces();
	init_screen_textures();

	current_screen.index = GAME_SCREEN_TITLE_INDEX;
	current_screen.should_run_game = GAME_SCREEN_TITLE_RUNS_GAME;

	init_ball_dimensions();
	init_ball_positions();

	init_paddles_misc();
	init_paddles_dimensions();
	init_paddles_positions();
}

/// <summary>
///		Reinitializes the game
/// </summary>
void reinit()
{
	init_ball_positions();
	init_paddles_positions();
}

/// <summary>
///		Sets up our game objects and any other initial data
/// </summary>
void setup()
{
	init();
}

void increment_score(int winning_player_index, int points)
{
	struct player* winning_player = &current_round.players[winning_player_index];
	current_round.round_num += 1;
	winning_player->score.points += points;
}

void reset_score()
{
	init_scoreboard();
}

/// <summary>
///		Determines whether the paddle could move to the proposed coordinates within play area bounds
/// </summary>
/// <param name="testX"></param>
/// <param name="testY"></param>
/// <returns></returns>
int can_move_paddle(float testX, float testY)
{
	if (testX < 0 || testX + PADDLE_WIDTH >= WINDOW_WIDTH ||
		testY < 0 || testY + PADDLE_HEIGHT >= WINDOW_HEIGHT)
		return FALSE;

	return TRUE;
}

/// <summary>
///		Determines whether the ball could move to the proposed coordinates within play area bounds
/// </summary>
/// <param name="testX"></param>
/// <param name="testY"></param>
/// <returns></returns>
int can_move_ball(float testX, float testY)
{
	if (testX < 0 || testX + BALL_SIZE >= WINDOW_WIDTH ||
		testY < 0 || testY + BALL_SIZE >= WINDOW_HEIGHT)
		return FALSE;

	return TRUE;
}

/// <summary>
///		Determines whether the ball and paddle are touching
/// </summary>
/// <param name="paddle"></param>
/// <param name="ball"></param>
/// <returns></returns>
int are_ball_paddle_touching(struct paddle* paddle, struct ball* ball)
{
	float minX = paddle->x;
	float maxX = paddle->x + PADDLE_WIDTH;
	float minY = paddle->y;
	float maxY = paddle->y + PADDLE_HEIGHT;

	if ((ball->x + BALL_SIZE >= minX && ball->x <= maxX) &&
		(ball->y + BALL_SIZE >= minY && ball->y <= maxY))
		return TRUE;

	return FALSE;
}

/// <summary>
///		Processes user input
/// </summary>
void process_input()
{
	SDL_Event event;
	SDL_PollEvent(&event);

	switch (event.type) {
		case SDL_QUIT:
			is_game_running = FALSE;
			break;

		case SDL_KEYDOWN: {
			if (event.key.keysym.sym == SDLK_ESCAPE)
			{
				is_game_running = FALSE;
				break;
			}

			if (event.key.keysym.sym == SDLK_SPACE && current_screen.index != GAME_SCREEN_GAME_INDEX)
			{
				current_screen.index = GAME_SCREEN_GAME_INDEX;
				current_screen.should_run_game = TRUE;
				break;
			}

			if (event.key.keysym.sym == SDLK_F1)
			{
				reset_score();
				reinit();
				current_screen.index = GAME_SCREEN_TITLE_INDEX;
				current_screen.should_run_game = FALSE;
				break;
			}

			// NOTE: currently we only catch the first key pressed
			// Game object inputs
			// Controller 0 = a/s
			// Controller 1 = up/down
			 
			// kept in for reference
			//if (event.key.keysym.sym == SDLK_UP || event.key.keysym.sym == SDLK_w)
			//{
			//	struct paddle paddle_for_controller = event.key.keysym.sym == SDLK_UP ? paddles[1] : paddles[0];
			//	last_input_keydown_sym = event.key.keysym.sym;
			//	can_move_paddle_to_new_pos = can_move_paddle(paddle_for_controller.x, paddle_for_controller.y - 1);
			//	break;
			//}
		}
	}

	// NOTE: credit goes to Casey Muratori for this Handmade Hero input system
	// I prefer handling capturing the input here and actually updating game objects in update code later for the frame, just feels more self-contained
	// We only support keyboard at the moment so this is dead simple
	const Uint8* keyboard_state = SDL_GetKeyboardState(NULL);

	for (size_t i = 0; i < GAME_CONTROLLERS_MAX; i++)
	{
		struct game_controller_input* old_controller = &old_input->controllers[i]; // controller input from last frame
		struct game_controller_input* new_controller = &new_input->controllers[i]; // current
		new_controller->is_analogue = FALSE;

		// If we had tons more buttons we could move this snippet for each button to its own func but this is fine for now
		new_controller->move_up.ended_down = i == 0 && keyboard_state[SDL_SCANCODE_W] || i == 1 && keyboard_state[SDL_SCANCODE_UP] ? 1 : 0;
		new_controller->move_up.half_transition_count = old_controller->move_up.ended_down != new_controller->move_up.ended_down ? 1 : 0;

		new_controller->move_down.ended_down = i == 0 && keyboard_state[SDL_SCANCODE_S] || i == 1 && keyboard_state[SDL_SCANCODE_DOWN] ? 1 : 0;
		new_controller->move_down.half_transition_count = old_controller->move_down.ended_down != new_controller->move_down.ended_down ? 1 : 0;
	}
}

/// <summary>
///		Retains input to the next frame
/// </summary>
void retain_input()
{
	struct game_input* temp = new_input;
	new_input = old_input;
	old_input = temp;
}

/// <summary>
///		Updates the state of our game objects
/// </summary>
void update()
{
	// ####################################
	//  FRAMERATE ENFORCEMENT
	// ####################################
	// todo: better to do this end of update cycle re: handmade hero flow? technically, this should include render time!
	// sleep the execution until we reach the target frame time in milliseconds
	int time_to_wait_ms = FRAME_TARGET_TIME_MS - (SDL_GetTicks() - last_frame_time_ms);

	// only call delay if we are too fast to process this frame
	if (time_to_wait_ms > 0 && time_to_wait_ms <= FRAME_TARGET_TIME_MS)
		SDL_Delay(time_to_wait_ms);

	// Get a delta time factor converted to seconds to be used to update my objects later
	float delta_time = (SDL_GetTicks() - last_frame_time_ms) / 1000.0f; // in seconds
	last_frame_time_ms = SDL_GetTicks();
	current_round.elapsed_ms += last_frame_time_ms;

	// ####################################
	//  GAMESCREEN STATE HANDLING
	// ####################################
	if (current_screen.index == GAME_SCREEN_TITLE_INDEX || current_screen.index == GAME_SCREEN_GAME_OVER_INDEX)
		return;

	// ####################################
	//  BALL POSITIONING
	// ####################################
	int new_ball_x_delta = ball.dx;
	int new_ball_y_delta = ball.dy;

	// Move ball by its motion vector
	ball.x += new_ball_x_delta;
	ball.y += new_ball_y_delta;
	
	// Bounces off the left/right goals (increment score, reset game)
	if (ball.x < 0 || ball.x > WINDOW_WIDTH - BALL_SIZE) {
		const struct player* player_zero = &current_round.players[0];
		const struct player* player_one = &current_round.players[1];
		int scoring_player_index = ball.x > WINDOW_WIDTH - BALL_SIZE ? 0 : 1;

		increment_score(scoring_player_index, SCORE_POINTS_INCREMENT);

		// winrar is u?
		if (player_zero->score.points == SCORE_MAX || player_one->score.points == SCORE_MAX)
		{
			current_screen.index = GAME_SCREEN_GAME_OVER_INDEX;
			current_screen.should_run_game = FALSE;
			return;
		}

		reinit();
	}

	// Bounces off top/bottom (turn the ball around)
	if (ball.y < 0 || ball.y > WINDOW_HEIGHT - BALL_SIZE)
		ball.dy = -ball.dy;

	// ####################################
	//  PADDLES
	// ####################################
	for (size_t i = 0; i < PADDLES_NUM_MAX; i++)
	{
		struct paddle* paddle = &paddles[i];

		// ####################################
		//  HANDLE INPUT UPDATES
		// ####################################
		// at the moment just saying we have two keyboard control patterns on the same keyboard
		const struct game_controller_input* old_controller = &old_input->controllers[i];
		const struct game_controller_input* new_controller = &new_input->controllers[i];
 
		// ####################################
		//  PADDLE POSITIONING
		// ####################################
		int are_ball_paddle_objs_touching = are_ball_paddle_touching(paddle, &ball);
		int can_move_ball_to_new_pos = !are_ball_paddle_objs_touching && can_move_ball(ball.x + new_ball_x_delta, ball.y + new_ball_y_delta);

		// INPUT UPDATES
		// Again, if we support more buttons we can make some wee funcs for this but it's fine for now
		if (new_controller->move_up.ended_down && can_move_paddle(paddle->x, paddle->y - 1))
			paddle->y += -PADDLE_MOVE_DY;

		if (new_controller->move_down.ended_down && can_move_paddle(paddle->x, paddle->y + 1))
			paddle->y += PADDLE_MOVE_DY;

		// ####################################
		//  BALL -> PADDLE COLLISION
		// ####################################
		// dx < 0 = moving left
		// dx > 0 = moving right
		// dy < 0 = moving up
		// dy > 0 = moving down

		// NOTE: I knew movement vectors were the answer here but was stuck on implementation
		// credit goes to https://github.com/flightcrank/pong/blob/master/pong.c for making this click for me
		if (are_ball_paddle_objs_touching)
		{
			// Moving left
			if (ball.dx < 0)
				ball.dx -= 1;
			// Moving right
			else
				ball.dx += 1;

			// change ball direction
			ball.dx = -ball.dx;

			// change ball angle based on where the paddle hit it
			int hit_pos = (paddle->y + paddle->height) - ball.y;

			if (hit_pos >= 0 && hit_pos < 7)
				ball.dy = 4;

			if (hit_pos >= 7 && hit_pos < 14)
				ball.dy = 3;

			if (hit_pos >= 14 && hit_pos < 21)
				ball.dy = 2;

			if (hit_pos >= 21 && hit_pos < 28)
				ball.dy = 1;

			if (hit_pos >= 28 && hit_pos < 32)
				ball.dy = 0;

			if (hit_pos >= 32 && hit_pos < 39)
				ball.dy = -1;

			if (hit_pos >= 39 && hit_pos < 46)
				ball.dy = -2;

			if (hit_pos >= 46 && hit_pos < 53)
				ball.dy = -3;

			if (hit_pos >= 53 && hit_pos <= 60)
				ball.dy = -4;

			// NOTE:
			// if the ball is bouncing into a paddle it can end
			// up trapped within the paddle geometry
			// so we ensure it's snapped outside the paddle shape bounds
			// to avoid a multi-collision bug

			// ball bouncing out to right (left paddle)
			if (ball.dx > 0 && ball.x <= PADDLES_X_OFFSET)
				ball.x = PADDLES_X_OFFSET;

			// ball bouncing out to left (right paddle)
			if(ball.dx <= 0 && ball.x + BALL_SIZE >= WINDOW_WIDTH - PADDLES_X_OFFSET - PADDLE_WIDTH)
				ball.x = WINDOW_WIDTH - PADDLES_X_OFFSET - PADDLE_WIDTH - BALL_SIZE;
		}
	}
}

void render_title_screen()
{
	if (current_screen.index != GAME_SCREEN_TITLE_INDEX)
		return;

	SDL_Rect src;
	SDL_Rect dest;

	src.x = src.y = 0;
	src.w = title_screen->w;
	src.h = title_screen->h;

	dest.x = (screen_surface->w / 2) - (src.w / 2);
	dest.y = (screen_surface->h / 2) - (src.h / 2);
	dest.w = title_screen->w;
	dest.h = title_screen->h;

	SDL_BlitSurface(title_screen, &src, screen_surface, &dest);
}

void render_ball()
{
	SDL_Rect ball_rect = {
		(int)ball.x,
		(int)ball.y,
		(int)ball.width,
		(int)ball.height
	};

	SDL_FillRect(screen_surface, &ball_rect, 0xFFFFFFFF);
}

void render_player_zero_paddle()
{
	SDL_Rect player_zero_paddle_rect = {
		(int)paddles[0].x,
		(int)paddles[0].y,
		(int)paddles[0].width,
		(int)paddles[0].height,
	};

	SDL_FillRect(screen_surface, &player_zero_paddle_rect, 0xFFFFFFFF);
}

void render_player_one_paddle()
{
	SDL_Rect player_one_paddle_rect = {
		(int)paddles[1].x,
		(int)paddles[1].y,
		(int)paddles[1].width,
		(int)paddles[1].height,
	};

	SDL_FillRect(screen_surface, &player_one_paddle_rect, 0xFFFFFFFF);
}

void render_net()
{
	int num_dashes_and_gaps = (NET_NUM_DASHES * 2);

	SDL_Rect net_dash;
	net_dash.w = NET_DASH_WIDTH;
	net_dash.h = screen_surface->h / num_dashes_and_gaps;
	net_dash.x = screen_surface->w / 2;
	net_dash.y = net_dash.h;

	int dash_offset = net_dash.h * 2;

	for (size_t i = 0; i < NET_NUM_DASHES; i++)
	{
		SDL_FillRect(screen_surface, &net_dash, 0xFFFFFFFF);
		net_dash.y += dash_offset;
	}
}

void render_player_zero_score() 
{
	const struct score* score = &current_round.players[0].score;

	SDL_Rect src;
	SDL_Rect dest;

	src.x = src.y = 0;
	src.w = SCORE_NUM_SPRITE_WIDTH;
	src.h = SCORE_NUM_SPRITE_WIDTH;

	dest.x = (screen_surface->w / 2) - src.w - SCORE_NUM_SPRITE_PADDING;
	dest.y = 0;
	dest.w = SCORE_NUM_SPRITE_WIDTH;
	dest.h = SCORE_NUM_SPRITE_HEIGHT;

	// A score higher than NINE, are you insane!?
	if (score->points > 0 && score->points < 10)
		src.x += src.w * score->points;

	SDL_BlitSurface(game_screen_num_map, &src, screen_surface, &dest);
}

void render_player_one_score()
{
	const struct score* score = &current_round.players[1].score;

	SDL_Rect src;
	SDL_Rect dest;

	src.x = src.y = 0;
	src.w = SCORE_NUM_SPRITE_WIDTH;
	src.h = SCORE_NUM_SPRITE_WIDTH;

	dest.x = (screen_surface->w / 2) + SCORE_NUM_SPRITE_PADDING - (SCORE_NUM_SPRITE_PADDING / 2);
	dest.y = 0;
	dest.w = SCORE_NUM_SPRITE_WIDTH;
	dest.h = SCORE_NUM_SPRITE_HEIGHT;

	// A score higher than NINE, are you insane!?
	if (score->points > 0 && score->points < 10)
		src.x += src.w * score->points;

	SDL_BlitSurface(game_screen_num_map, &src, screen_surface, &dest);
}

void render_scores()
{
	render_player_zero_score();
	render_player_one_score();
}

void render_game_screen()
{
	// Game objects & play field
	render_ball();
	render_player_zero_paddle();
	render_player_one_paddle();
	render_net();

	// Scores/timer etc.
	render_scores();
}

void render_game_over_screen()
{
	if (current_screen.index != GAME_SCREEN_GAME_OVER_INDEX)
		return;

	// simple enough to assume if we're rendering this we've got a clear winner
	const struct player* player_zero = &current_round.players[0];
	const struct player* player_one = &current_round.players[1];
	const int winning_player_index = player_zero->score.points > player_one->score.points ? 0 : 1;

	SDL_Rect player_zero_msg;
	SDL_Rect player_one_msg;
	SDL_Rect dest;

	player_zero_msg.x = player_zero_msg.y = player_one_msg.x = 0;
	player_zero_msg.w = player_one_msg.w = game_over_screen->w;
	player_zero_msg.h = player_one_msg.h = GAME_SCREEN_GAME_OVER_MSG_HEIGHT;

	player_one_msg.y = GAME_SCREEN_GAME_OVER_MSG_OFFSET;

	dest.x = (screen_surface->w / 2) - (game_over_screen->w / 2);
	dest.y = (screen_surface->h / 2) - (GAME_SCREEN_GAME_OVER_MSG_HEIGHT / 2);
	dest.w = game_over_screen->w;
	dest.h = game_over_screen->h;
	
	// switch if we add more like AI
	if (winning_player_index == 0)
	{
		SDL_BlitSurface(game_over_screen, &player_zero_msg, screen_surface, &dest);
		return;
	}

	SDL_BlitSurface(game_over_screen, &player_one_msg, screen_surface, &dest);
}

/// <summary>
///		Renders our game screens
/// </summary>
void render()
{
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_FillRect(screen_surface, NULL, 0x00000000);
	SDL_RenderClear(renderer);

	switch (current_screen.index) {
		case GAME_SCREEN_TITLE_INDEX: 
			render_title_screen();
			break;

		case GAME_SCREEN_GAME_INDEX:
			render_game_screen();
			break;

		case GAME_SCREEN_GAME_OVER_INDEX:
			render_game_over_screen();
			break;
	}

	SDL_UpdateTexture(screen_texture, NULL, screen_surface->pixels, screen_surface->w * sizeof(Uint32));
	SDL_RenderCopy(renderer, screen_texture, NULL, NULL);

	// swap the backbuffer with the current front buffer
	SDL_RenderPresent(renderer);
}

int main(int argc, char* args[])
{
	is_game_running = initialize_window();
	setup();

	while (is_game_running)
	{
		//check for new events every frame
		SDL_PumpEvents();

		process_input();
		update();
		retain_input();
		render();
	}

	destroy_window();

	return 0;
}