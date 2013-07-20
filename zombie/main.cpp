#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <time.h>
#include <math.h>
#include <list>
#include <iostream>
#include <stdio.h>

#define PI 3.141592653

using namespace std;

const int width = 1920;
const int height = 1080;


class Entity {
	public:
		Entity(double x, double y): x(x), y(y), vx(0), ax(0), vy(0), ay(0), radius(10) {};

		virtual void move(double time, list<Entity*> entities) {
			vx += ax*time;
			vy += ay*time;
			x += vx*time + ax*pow(time, 2)/2;
			y += vy*time + ay*pow(time, 2)/2;

			while(x >= width){
				x -= width;
			}

			while(y >= height){
				y -= height;
			}

			while(x < 0){
				x += width;
			}

			while(y < 0){
				y += height;
			}

			for(list<Entity*>::iterator it = entities.begin(); it != entities.end(); ++it){
				Entity *e = *it;
				if(e == this){
					continue;
				}
				//de-overlap
				double d = getDistance(e);
				if(d < 2*radius){
					x += -(x - e->getX()) + 2*radius*(x - e->getX())/d;
					y += -(y - e->getY()) + 2*radius*(y - e->getY())/d;
				}
			}
		}

		virtual double getX(){
			return x;
		}

		virtual double getY(){
			return y;
		}

		virtual void doAI(list<Entity*> entities) = 0;
		virtual void draw(sf::RenderWindow &w) = 0;
		virtual string getType() = 0;

		double getDistance(Entity *other){
			return sqrt(pow(x - other->getX(), 2) + pow(y - other->getY(), 2));
		}
			
	protected:
		double x;
		double y;
		double vx;
		double vy;
		double ax;
		double ay;
		double radius;
};

class Human: public Entity {
	public:
		enum Type {
			Leader,
			Follower,
			FatGuy,
			Suicidal,
			Speedy,
			RunFromClosest,
			StayTogether,
		};
	
		Human(double x, double y): Entity(x, y), maxSpeed(50), maxAcceleration(100) {
			type = (Type) (rand() % 2);
			createGeometry();
		};
		
		Human(double x, double y, Type type): Entity(x, y), type(type), maxSpeed(50), maxAcceleration(100) {
			createGeometry();
		};

		virtual void doAI(list<Entity*> entities){

			if(type == Type::Follower){
				//try to follow the closest leader
				Human *closest = 0;
				double dToClosest = 0;
				for(list<Entity*>::iterator it = entities.begin(); it != entities.end(); ++it){
					Entity *e = *it;
					if(e == this){
						continue;
					}
					if(e->getType() == "human"){
						Human *h = static_cast<Human*> (e);
						if(h->getAItype() == Type::Leader){
							double d = getDistance(e);
							if(closest == 0 || dToClosest > d){
								closest = h;
								dToClosest = d;
							}
						}
					}
				}

				if(closest && dToClosest){
					ax = maxAcceleration*(closest->getX() - x)/dToClosest;
					ay = maxAcceleration*(closest->getY() - y)/dToClosest;
				}
			}else{ //leader
				if(scheduledChange < lifetime.getElapsedTime()){
					scheduledChange = lifetime.getElapsedTime() + sf::seconds(rand()%5 + 1);
					ax = rand()%((int) maxAcceleration) * (rand()%2 ? 1 : -1);
					ay = rand()%((int) maxAcceleration) * (rand()%2 ? 1 : -1);
				}
			}
	
			double mod = sqrt(pow(vx, 2) + pow(vy, 2)) ;

			if(mod > maxSpeed){
				vx *= maxSpeed/mod;
				vy *= maxSpeed/mod;
			}
		}

		virtual void move(double time, list<Entity*> entities){
			vx += ax*time;
			vy += ay*time;
			x += vx*time + ax*pow(time, 2)/2;
			y += vy*time + ay*pow(time, 2)/2;

			while(x >= width){
				x -= width;
			}

			while(y >= height){
				y -= height;
			}

			while(x < 0){
				x += width;
			}

			while(y < 0){
				y += height;
			}

			if(type != Type::Leader){
				for(list<Entity*>::iterator it = entities.begin(); it != entities.end(); ++it){
					Entity *e = *it;
					if(e == this){
						continue;
					}
					//de-overlap
					double d = getDistance(e);
					if(d < 2*radius){
						x += -(x - e->getX()) + 2*radius*(x - e->getX())/d;
						y += -(y - e->getY()) + 2*radius*(y - e->getY())/d;
					}
				}
			}

			geometry.setPosition(x, y);
			
			/*if(vx != 0){
				lookAngle = atan(vy/vx);
			}*/
			//geometry.setRotation(lookAngle*180/PI);
		}

		virtual void draw(sf::RenderWindow &w){
			w.draw(geometry);
		}

		Type getAItype(){
			return type;
		}

		virtual string getType() {
			return "human";
		}

	private:
		Type type;
		sf::Time scheduledChange;
		sf::Clock lifetime;
		sf::CircleShape geometry;
		sf::Color fill;
		sf::Color outline;
		float lookAngle;
		double maxSpeed;
		double maxAcceleration;

		void createGeometry() {
			fill = type == Type::Follower ? sf::Color(198, 156, 109, 255) : sf::Color(0, 191, 243, 255);
			outline = type == Type::Follower ?  sf::Color(166, 124, 82, 255) : sf::Color(64, 140, 203, 255);

			geometry.setRadius(radius);
			geometry.setPointCount(6);
			geometry.setFillColor(fill);
			geometry.setOutlineColor(outline);
			geometry.setOutlineThickness(2);
			geometry.setOrigin(radius, radius);
		}
};

int main(){

	srand(time(NULL));

	sf::VideoMode vm(width, height);
	sf::ContextSettings ctxs;
	ctxs.antialiasingLevel = 2;
	sf::RenderWindow window(vm, "Zombiefication", sf::Style::Fullscreen, ctxs);

	window.setVerticalSyncEnabled(true);

	list<Entity*> humans;

	for(int i = 0; i < 50; i++){
		Human *h = new Human(rand()%width, rand()%height, rand()%4 ? Human::Type::Follower : Human::Type::Leader);
		cout << "Creating a human\n";
		humans.push_back(h);
	}

	sf::Clock k;
	double t;
	while(window.isOpen()){
		sf::Event e;
		while(window.pollEvent(e)){
			if(e.type == sf::Event::Closed){
				window.close();
			}
		}

		t = k.restart().asSeconds();

		for(list<Entity*>::iterator it = humans.begin(); it != humans.end(); ++it){
			(*it)->doAI(humans);
			(*it)->move(t, humans);
		}

		window.clear();

		for(list<Entity*>::iterator it = humans.begin(); it != humans.end(); ++it){
			(*it)->draw(window);
		}

		window.display();
	}

	return 0;
}