using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class snowballStick : MonoBehaviour
{
    private bool isStuck = false;
    private Vector3 colPoint;
    private GameObject golfBall;
    // Start is called before the first frame update
    void Start()
    {
        
    }

    // Update is called once per frame
    void Update()
    {
        if (isStuck)
        {
           Physics.IgnoreCollision(golfBall.GetComponent<Collider>(), GetComponent<Collider>(), true);
           transform.position = colPoint;
           
        }
    }

    void OnCollisionEnter(Collision collision)
    {
        if (collision.gameObject.tag.Equals("ball"))
        {
            golfBall = collision.gameObject;
           
           FixedJoint sbJoint = gameObject.AddComponent<FixedJoint>() as FixedJoint;
           Rigidbody rb = GetComponent<Rigidbody>();
           sbJoint.connectedBody = golfBall.gameObject.GetComponent<Rigidbody>();
           
            rb.mass = 0.00001f;
            rb.freezeRotation = true;
            rb.velocity = new Vector3(0, 0, 0);
           
        }
    }
}
