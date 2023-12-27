#include <stdio.h>
#include <SDL.h>
#include "constants.h"

/*
 * #############################################
 *  TYPE DEFS
 * #############################################
 */
struct ball
{
	float x;
	float y;
	float width;
	float height;
	int dx; // movement vector
	int dy; // movement vector
} ball;

struct paddle
{
	float x;
	float y;
	float width;
	float height;
} paddle;

struct round
{
	int round_num;
	int elapsed_ms;
	int points;
};

struct scoreboard
{
	struct round* rounds;
};

/*
 * #############################################
 *  GLOBALS
 * #############################################
 */

int is_game_running = FALSE;
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
int last_frame_time_ms = 0;

uint32_t last_input_keydown_sym = 0;

int can_move_paddle_to_new_pos = FALSE;
int can_move_ball_to_new_pos = FALSE;
int are_ball_paddle_objs_touching = FALSE;

struct scoreboard game_scoreboard;
struct round current_round;

/// <summary>
///		Initializes our window and renderer
/// </summary>
/// <param name=""></param>
/// <returns></returns>
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
///		Initializes the game
/// </summary>
void init()
{
	current_round.elapsed_ms = 0;
	current_round.points = 0;
	current_round.round_num = 0;

	ball.x = 20;
	ball.y = 400;
	ball.width = BALL_SIZE;
	ball.height = BALL_SIZE;
	ball.dx = 1;
	ball.dy = 1;

	paddle.width = PADDLE_WIDTH;
	paddle.height = PADDLE_HEIGHT;
	paddle.x = (WINDOW_WIDTH / 2) - (paddle.width / 2);
	paddle.y = WINDOW_HEIGHT - 30;
}

/// <summary>
///		Reinitializes the game
/// </summary>
void reinit()
{
	ball.x = 20;
	ball.y = 400;
	ball.dx = ball.dy = 1;

	paddle.x = (WINDOW_WIDTH / 2) - (paddle.width / 2);
	paddle.y = WINDOW_HEIGHT - 30;
}

/// <summary>
///		Sets up our game objects and any other initial data
/// </summary>
void setup()
{
	init();
}

/// <summary>
///		Determines whether the paddle could move to the proposed coordinates within play area bounds
/// </summary>
/// <param name="testX"></param>
/// <param name="testY"></param>
/// <returns></returns>
int can_move_paddle(float testX, float testY)
{
	if (testX < 0 || testX > WINDOW_WIDTH - PADDLE_WIDTH ||
		testY < 0 || testY > WINDOW_HEIGHT + PADDLE_HEIGHT)
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
	if (testX < 0 || testX + BALL_SIZE > WINDOW_WIDTH ||
		testY < 0 || testY + BALL_SIZE > WINDOW_HEIGHT)
		return FALSE;

	return TRUE;
}

/// <summary>
///		Determines whether the ball and paddle are touching
/// </summary>
/// <returns></returns>
int are_ball_paddle_touching()
{
	if ((ball.x >= paddle.x && ball.x <= paddle.x + PADDLE_WIDTH) && ball.y + BALL_SIZE >= paddle.y)
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

			// Game object inputs
			if (event.key.keysym.sym == SDLK_LEFT) 
			{
				last_input_keydown_sym = SDLK_LEFT;
				can_move_paddle_to_new_pos = can_move_paddle(paddle.x - 1, paddle.y);
				break;
			}

			if (event.key.keysym.sym == SDLK_RIGHT)
			{
				last_input_keydown_sym = SDLK_RIGHT;
				can_move_paddle_to_new_pos = can_move_paddle(paddle.x + 1, paddle.y);
				break;
			}
		}
	}
}

/// <summary>
///		Updates the state of our game objects
/// </summary>
void update()
{
	// ####################################
	//  FRAMERATE ENFORCEMENT
	// ####################################
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
	//  BALL
	// ####################################
	int new_ball_x_delta = ball.dx;
	int new_ball_y_delta = ball.dy;

	// Move ball by its motion vector
	ball.x += new_ball_x_delta;
	ball.y += new_ball_y_delta;

	// Bounces off the sides (turn the ball around)
	if (ball.x < 0 || ball.x > WINDOW_WIDTH - BALL_SIZE) 
		ball.dx = -ball.dx;

	// Bounces off top (turn the ball around)
	if (ball.y < 0) 
		ball.dy = -ball.dy;

	// Hit the bottom, reset game
	if (ball.y > WINDOW_HEIGHT - BALL_SIZE)
		reinit();

	// Ball collision movement vector tweaks
	// dx < 0 = moving left
	// dx > 0 = moving right
	// dy < 0 = moving up
	// dy > 0 = moving down
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
		int hit_pos = (paddle.y + paddle.height) - ball.y;

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

		//ball moving right
		//teleport ball to avoid mutli collision glitch
		if (ball.dx > 0 && ball.x < 30) 
		{
			ball.x = 30;
		}
		//ball moving left
		else 
		{
			//teleport ball to avoid mutli collision glitch
			if (ball.x > 600)
				ball.x = 600;
		}
	}

	// ####################################
	//  PADDLE
	// ####################################
	are_ball_paddle_objs_touching = are_ball_paddle_touching();
	can_move_ball_to_new_pos = !are_ball_paddle_objs_touching && can_move_ball(ball.x + new_ball_x_delta, ball.y + new_ball_y_delta);

	if (can_move_paddle_to_new_pos)
	{
		paddle.x += last_input_keydown_sym == SDLK_LEFT ? -10 : 10;
		last_input_keydown_sym = 0;
		can_move_paddle_to_new_pos = FALSE;
	}
}

/// <summary>
///		Renders our game objects
/// </summary>
void render()
{
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);

	SDL_Rect ball_rect = {
		(int)ball.x,
		(int)ball.y,
		(int)ball.width,
		(int)ball.height
	};

	SDL_Rect paddle_rect = {
		(int)paddle.x,
		(int)paddle.y,
		(int)paddle.width,
		(int)paddle.height
	};

	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	SDL_RenderFillRect(renderer, &ball_rect);
	SDL_RenderFillRect(renderer, &paddle_rect);

	// swap the backbuffer with the current front buffer
	SDL_RenderPresent(renderer);
}

int main(int argc, char* args[])
{
	is_game_running = initialize_window();
	setup();

	while (is_game_running)
	{
		process_input();
		update();
		render();
	}

	destroy_window();

	return 0;
}