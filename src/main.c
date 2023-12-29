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

struct paddle paddles[2];
struct ball ball;

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

	paddles[0].controller_index = 0;
	paddles[1].controller_index = 1;
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
	current_round.elapsed_ms = 0;
	current_round.points = 0;
	current_round.round_num = 0;

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

			// NOTE: currently we only catch the first key pressed
			// Game object inputs
			// Controller 0 = a/s
			// Controller 1 = up/down
			if (event.key.keysym.sym == SDLK_UP || event.key.keysym.sym == SDLK_w)
			{
				struct paddle paddle_for_controller = event.key.keysym.sym == SDLK_UP ? paddles[1] : paddles[0];
				last_input_keydown_sym = event.key.keysym.sym;
				can_move_paddle_to_new_pos = can_move_paddle(paddle_for_controller.x, paddle_for_controller.y - 1);
				break;
			}

			if (event.key.keysym.sym == SDLK_DOWN || event.key.keysym.sym == SDLK_s)
			{
				struct paddle paddle_for_controller = event.key.keysym.sym == SDLK_DOWN ? paddles[1] : paddles[0];
				last_input_keydown_sym = event.key.keysym.sym;
				can_move_paddle_to_new_pos = can_move_paddle(paddle_for_controller.x, paddle_for_controller.y + 1);
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
	//  BALL POSITIONING
	// ####################################
	int new_ball_x_delta = ball.dx;
	int new_ball_y_delta = ball.dy;

	// Move ball by its motion vector
	ball.x += new_ball_x_delta;
	ball.y += new_ball_y_delta;
	
	// Bounces off the left/right goals (increment score, reset game)
	if (ball.x < 0 || ball.x > WINDOW_WIDTH - BALL_SIZE)
		reinit();

	// Bounces off top/bottom (turn the ball around)
	if (ball.y < 0 || ball.y > WINDOW_HEIGHT - BALL_SIZE)
		ball.dy = -ball.dy;

	// ####################################
	//  PADDLES
	// ####################################
	for (size_t i = 0; i < 2; i++)
	{
		int is_paddle_for_controller_input = 0;

		switch (last_input_keydown_sym) {
			case SDLK_w:
			case SDLK_s: {
				is_paddle_for_controller_input = i == 0;
				break;
			}

			case SDLK_UP:
			case SDLK_DOWN: {
				is_paddle_for_controller_input = i == 1;
				break;
			}
		}

		// ####################################
		//  PADDLE POSITIONING
		// ####################################
		are_ball_paddle_objs_touching = are_ball_paddle_touching(&paddles[i], &ball);
		can_move_ball_to_new_pos = !are_ball_paddle_objs_touching && can_move_ball(ball.x + new_ball_x_delta, ball.y + new_ball_y_delta);

		if (can_move_paddle_to_new_pos && is_paddle_for_controller_input)
		{
			paddles[i].y += last_input_keydown_sym == SDLK_UP || last_input_keydown_sym == SDLK_w ? -PADDLE_MOVE_DY : PADDLE_MOVE_DY;
			last_input_keydown_sym = 0;
			can_move_paddle_to_new_pos = FALSE;
		}

		// ####################################
		//  BALL -> PADDLE COLLISION
		// ####################################
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
			int hit_pos = (paddles[i].y + paddles[i].height) - ball.y;

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

	SDL_Rect player_one_paddle_rect = {
		(int)paddles[0].x,
		(int)paddles[0].y,
		(int)paddles[0].width,
		(int)paddles[0].height,
	};

	SDL_Rect player_two_paddle_rect = {
		(int)paddles[1].x,
		(int)paddles[1].y,
		(int)paddles[1].width,
		(int)paddles[1].height,
	};

	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	SDL_RenderFillRect(renderer, &ball_rect);
	SDL_RenderFillRect(renderer, &player_one_paddle_rect);
	SDL_RenderFillRect(renderer, &player_two_paddle_rect);

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
		render();
	}

	destroy_window();

	return 0;
}