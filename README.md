# Snake

![image](https://user-images.githubusercontent.com/28790865/117322335-ea3c2800-ae5b-11eb-8f59-c0eb6a1e66c3.png)



## IMPORTANT: 
X11 needs be installed to execute this application. Please install **Xming** for Windows or **XQuartz** for macOS. 
For macOS users, after installing XQuartz, you may add a symlink to your X11 installation folder by just entering
**ln -s /opt/X11/include/X11 /usr/local/include/X11**  command.



### ** Instruction to execute **

1. Compile the file by 'make' commend
2. Enter this execution command: ./snake [FPS] [snake_speed] 

Recommended test setting FPS = 25 - 60, speed = 3, 4, or 5



### ** Enhancement **

	: Invincible and Double score portions. See item description below.


#### 1. Objective: 

	Earn the highest score by eating as many fruit as you can.


#### 2. Key Control:

	Move and change direction using arrow keys.

	s: Start game from initial screen.

	p: Pause game during play. Press p again to resume the game.

	r: Restart game

		After pressing r:

		y: Restart 

		n: Resume and return to game.

	q: Quit game.

#### 3. Change Snake's Direction:

	Snake moves towards the direction of the arrow key pressed. However, it cannot turn 180 degrees. Only turn 90 degree turns are avaliable (left and right turns).

#### 4. Snake Dies When:

	It crashes into any of the four walls.

	Snake head eats its own body (unless it has previously ate an invicible potion).


#### 5. Items:

	Fruit: 

		Depicted as a green square on the screen. 

		+1 score when snake eats it.

		Regenerated in random places after it is eaten. Edge corners are off limits for regeneration.

	Invincible potion (**This is an enhancement**):

		Depicted as a blue square on the screen.

		Generated by a probability set.

		After eaten, snake becomes invincible and will not die. Snake will be able to pass through walls and its own body.

		Its effect lasts for 30 seconds.

		Background colour changes to cyan while snake is invincible.

		Background colour changes to orange when the effective time is 10 seconds remaining.

		Background colour changes back to yellow again when the effect is gone.

	Double score potion (**This is an enhancement**):

		Depicted as brown square on the screen.

		When this potion is in effect, all fruit will give double points when eaten.

		Background colour changes in the same way with the invincible potion, but will turn magenta instead of cyan. 

#### 5. Growing Snake:

	With each fruit eaten, the snake's body will increase in length by (moving speed * 5). Thus care should be taken when eating fruit near walls. This is why items are not generated on the corner edges.



#### 6. Others:

	a. Initial snake size and how much it grows depend on the moving speed.

		Upon game play, the initial snake length equals 10 overlapped squares. Whenever a fruit is eaten, the snake will grow 5 additional pieces. The snake is made up of squares overlapped on top of one another to achieve a long cohesive snake. Thus when it grows 5 additional pieces, 5 squares will be added on to the last square, overlapped to elongate the length. The gap between the two overlapped squares is equal to the moving speed.

	b. Snake's body looks black when speed is 1 or 2.

		Since the moving speed is 1 or 2, the whole square is almost overlapped except for length 1 or 2. This is just enough to show the black edges of each of the squares as the red colour will be overlapped. This causes the snake body to be black instead of red.

